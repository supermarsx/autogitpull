#include "git_utils.hpp"
#include <git2.h>
#include <functional>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace std;

namespace git {

struct ProgressData {
    const std::function<void(int)>* cb;
    std::chrono::steady_clock::time_point start;
    size_t down_limit;
    size_t up_limit;
};

static int credential_cb(git_credential** out, const char* url, const char* username_from_url,
                         unsigned int allowed_types, void* payload) {
    const char* user = getenv("GIT_USERNAME");
    const char* pass = getenv("GIT_PASSWORD");

    if ((allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) && user && pass) {
        return git_credential_userpass_plaintext_new(out, user, pass);
    }
    return git_credential_default_new(out);
}

GitInitGuard::GitInitGuard() { git_libgit2_init(); }

GitInitGuard::~GitInitGuard() { git_libgit2_shutdown(); }

bool is_git_repo(const fs::path& p) {
    return fs::exists(p / ".git") && fs::is_directory(p / ".git");
}

static string oid_to_hex(const git_oid& oid) {
    char buf[GIT_OID_HEXSZ + 1];
    git_oid_tostr(buf, sizeof(buf), &oid);
    return string(buf);
}

string get_local_hash(const fs::path& repo) {
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0)
        return "";
    git_oid oid;
    if (git_reference_name_to_id(&oid, r, "HEAD") != 0) {
        git_repository_free(r);
        return "";
    }
    string hash = oid_to_hex(oid);
    git_repository_free(r);
    return hash;
}

string get_current_branch(const fs::path& repo) {
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0)
        return "";
    git_reference* head = nullptr;
    if (git_repository_head(&head, r) != 0) {
        git_repository_free(r);
        return "";
    }
    const char* name = git_reference_shorthand(head);
    string branch = name ? name : "";
    git_reference_free(head);
    git_repository_free(r);
    return branch;
}

string get_remote_hash(const fs::path& repo, const string& branch, bool use_credentials,
                       bool* auth_failed) {
    git_repository* r = nullptr;

    if (git_repository_open(&r, repo.string().c_str()) != 0)
        return "";
    git_remote* remote = nullptr;
    if (git_remote_lookup(&remote, r, "origin") != 0) {
        git_repository_free(r);
        return "";
    }
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    if (use_credentials)
        fetch_opts.callbacks.credentials = credential_cb;
    int err = git_remote_fetch(remote, nullptr, &fetch_opts, nullptr);
    if (err != 0) {
        const git_error* e = git_error_last();
        if (auth_failed && e && e->message &&
            std::string(e->message).find("auth") != std::string::npos)
            *auth_failed = true;
    }
    git_remote_free(remote);
    git_oid oid;
    string refname = string("refs/remotes/origin/") + branch;
    if (git_reference_name_to_id(&oid, r, refname.c_str()) != 0) {
        git_repository_free(r);
        return "";
    }
    string hash = oid_to_hex(oid);
    git_repository_free(r);
    return hash;
}

string get_origin_url(const fs::path& repo) {
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0)
        return "";
    git_remote* remote = nullptr;
    if (git_remote_lookup(&remote, r, "origin") != 0) {
        git_repository_free(r);
        return "";
    }
    const char* url = git_remote_url(remote);
    string result = url ? url : "";
    git_remote_free(remote);
    git_repository_free(r);
    return result;
}

bool is_github_url(const string& url) { return url.find("github.com") != string::npos; }

bool remote_accessible(const fs::path& repo) {
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0)
        return false;
    git_remote* remote = nullptr;
    if (git_remote_lookup(&remote, r, "origin") != 0) {
        git_repository_free(r);
        return false;
    }
    int err = git_remote_connect(remote, GIT_DIRECTION_FETCH, nullptr, nullptr, nullptr);
    bool ok = err == 0;
    if (ok)
        git_remote_disconnect(remote);
    git_remote_free(remote);
    git_repository_free(r);
    return ok;
}

int try_pull(const fs::path& repo, string& out_pull_log,
             const std::function<void(int)>* progress_cb, bool use_credentials, bool* auth_failed,
             size_t down_limit_kbps, size_t up_limit_kbps) {


    if (progress_cb)
        (*progress_cb)(0);
    auto finalize = [&]() {
        if (progress_cb)
            (*progress_cb)(100);
    };

    git_repository* r = nullptr;

    if (git_repository_open(&r, repo.string().c_str()) != 0) {
        out_pull_log = "Failed to open repository";
        finalize();
        return 2;
    }
    string branch = get_current_branch(repo);
    git_remote* remote = nullptr;
    if (git_remote_lookup(&remote, r, "origin") != 0) {
        git_repository_free(r);
        out_pull_log = "No origin remote";
        finalize();
        return 2;
    }
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    ProgressData progress{progress_cb, std::chrono::steady_clock::now(), down_limit_kbps,
                          up_limit_kbps};
    if (progress_cb || down_limit_kbps > 0 || up_limit_kbps > 0) {
        callbacks.payload = &progress;
        callbacks.transfer_progress = [](const git_transfer_progress* stats, void* payload) -> int {
            if (!payload)
                return 0;
            auto* pd = static_cast<ProgressData*>(payload);
            if (pd->cb) {
                int pct = 0;
                if (stats->total_objects > 0)
                    pct = static_cast<int>(100 * stats->received_objects / stats->total_objects);
                (*pd->cb)(pct);
            }
            double expected_ms = 0.0;
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - pd->start)
                               .count();
            if (pd->down_limit > 0) {
                double ms = (double)stats->received_bytes / (pd->down_limit * 1024.0) * 1000.0;
                if (ms > expected_ms)
                    expected_ms = ms;
            }
            if (pd->up_limit > 0) {
                double ms = (double)stats->received_bytes / (pd->up_limit * 1024.0) * 1000.0;
                if (ms > expected_ms)
                    expected_ms = ms;
            }
            if (expected_ms > elapsed) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(static_cast<int>(expected_ms - elapsed)));
            }
            return 0;
        };
    }
    if (use_credentials)
        callbacks.credentials = credential_cb;
    fetch_opts.callbacks = callbacks;
    int err = git_remote_fetch(remote, nullptr, &fetch_opts, nullptr);
    if (err != 0) {
        const git_error* e = git_error_last();
        if (auth_failed && e && e->message &&
            std::string(e->message).find("auth") != std::string::npos)
            *auth_failed = true;
        git_remote_free(remote);
        git_repository_free(r);
        out_pull_log = "Fetch failed";
        finalize();
        return 2;
    }
    git_oid remote_oid;
    string refname = string("refs/remotes/origin/") + branch;
    if (git_reference_name_to_id(&remote_oid, r, refname.c_str()) != 0) {
        git_remote_free(remote);
        git_repository_free(r);
        out_pull_log = "Remote branch not found";
        finalize();
        return 2;
    }
    git_oid local_oid;
    if (git_reference_name_to_id(&local_oid, r, "HEAD") != 0) {
        git_remote_free(remote);
        git_repository_free(r);
        out_pull_log = "Local HEAD not found";
        finalize();
        return 2;
    }
    if (git_oid_cmp(&local_oid, &remote_oid) == 0) {
        out_pull_log = "Already up to date";
        git_remote_free(remote);
        git_repository_free(r);
        finalize();
        return 0;
    }
    git_object* target = nullptr;
    if (git_object_lookup(&target, r, &remote_oid, GIT_OBJECT_COMMIT) != 0) {
        git_remote_free(remote);
        git_repository_free(r);
        out_pull_log = "Lookup failed";
        finalize();
        return 2;
    }
    if (git_reset(r, target, GIT_RESET_HARD, nullptr) != 0) {
        git_object_free(target);
        git_remote_free(remote);
        git_repository_free(r);
        out_pull_log = "Reset failed";
        finalize();
        return 2;
    }
    git_object_free(target);
    git_remote_free(remote);
    git_repository_free(r);
    out_pull_log = "Fast-forwarded";
    finalize();
    return 0;
}

} // namespace git
