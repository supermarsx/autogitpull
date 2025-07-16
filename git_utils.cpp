#include "git_utils.hpp"
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
    git_repository* raw = nullptr;
    if (git_repository_open(&raw, repo.string().c_str()) != 0)
        return "";
    repo_ptr r(raw, git_repository_free);
    git_oid oid;
    if (git_reference_name_to_id(&oid, r.get(), "HEAD") != 0)
        return "";
    return oid_to_hex(oid);
}

string get_current_branch(const fs::path& repo) {
    git_repository* raw = nullptr;
    if (git_repository_open(&raw, repo.string().c_str()) != 0)
        return "";
    repo_ptr r(raw, git_repository_free);
    git_reference* head = nullptr;
    if (git_repository_head(&head, r.get()) != 0)
        return "";
    const char* name = git_reference_shorthand(head);
    string branch = name ? name : "";
    git_reference_free(head);
    return branch;
}

string get_remote_hash(const fs::path& repo, const string& branch, bool use_credentials,
                       bool* auth_failed) {
    git_repository* raw_repo = nullptr;

    if (git_repository_open(&raw_repo, repo.string().c_str()) != 0)
        return "";
    repo_ptr r(raw_repo, git_repository_free);
    git_remote* raw_remote = nullptr;
    if (git_remote_lookup(&raw_remote, r.get(), "origin") != 0)
        return "";
    remote_ptr remote(raw_remote, git_remote_free);
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    if (use_credentials)
        fetch_opts.callbacks.credentials = credential_cb;
    int err = git_remote_fetch(remote.get(), nullptr, &fetch_opts, nullptr);
    if (err != 0) {
        const git_error* e = git_error_last();
        if (auth_failed && e && e->message &&
            std::string(e->message).find("auth") != std::string::npos)
            *auth_failed = true;
    }
    git_oid oid;
    string refname = string("refs/remotes/origin/") + branch;
    if (git_reference_name_to_id(&oid, r.get(), refname.c_str()) != 0)
        return "";
    return oid_to_hex(oid);
}

string get_origin_url(const fs::path& repo) {
    git_repository* raw_repo = nullptr;
    if (git_repository_open(&raw_repo, repo.string().c_str()) != 0)
        return "";
    repo_ptr r(raw_repo, git_repository_free);
    git_remote* raw_remote = nullptr;
    if (git_remote_lookup(&raw_remote, r.get(), "origin") != 0)
        return "";
    remote_ptr remote(raw_remote, git_remote_free);
    const char* url = git_remote_url(remote.get());
    return url ? url : "";
}

bool is_github_url(const string& url) { return url.find("github.com") != string::npos; }

bool remote_accessible(const fs::path& repo) {
    git_repository* raw_repo = nullptr;
    if (git_repository_open(&raw_repo, repo.string().c_str()) != 0)
        return false;
    repo_ptr r(raw_repo, git_repository_free);
    git_remote* raw_remote = nullptr;
    if (git_remote_lookup(&raw_remote, r.get(), "origin") != 0)
        return false;
    remote_ptr remote(raw_remote, git_remote_free);
    int err = git_remote_connect(remote.get(), GIT_DIRECTION_FETCH, nullptr, nullptr, nullptr);
    bool ok = err == 0;
    if (ok)
        git_remote_disconnect(remote.get());
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

    git_repository* raw_repo = nullptr;

    if (git_repository_open(&raw_repo, repo.string().c_str()) != 0) {
        out_pull_log = "Failed to open repository";
        finalize();
        return 2;
    }
    repo_ptr r(raw_repo, git_repository_free);
    string branch = get_current_branch(repo);
    git_remote* raw_remote = nullptr;
    if (git_remote_lookup(&raw_remote, r.get(), "origin") != 0) {
        out_pull_log = "No origin remote";
        finalize();
        return 2;
    }
    remote_ptr remote(raw_remote, git_remote_free);
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
    int err = git_remote_fetch(remote.get(), nullptr, &fetch_opts, nullptr);
    if (err != 0) {
        const git_error* e = git_error_last();
        if (auth_failed && e && e->message &&
            std::string(e->message).find("auth") != std::string::npos)
            *auth_failed = true;
        out_pull_log = "Fetch failed";
        finalize();
        return 2;
    }
    git_oid remote_oid;
    string refname = string("refs/remotes/origin/") + branch;
    if (git_reference_name_to_id(&remote_oid, r.get(), refname.c_str()) != 0) {
        out_pull_log = "Remote branch not found";
        finalize();
        return 2;
    }
    git_oid local_oid;
    if (git_reference_name_to_id(&local_oid, r.get(), "HEAD") != 0) {
        out_pull_log = "Local HEAD not found";
        finalize();
        return 2;
    }
    if (git_oid_cmp(&local_oid, &remote_oid) == 0) {
        out_pull_log = "Already up to date";
        finalize();
        return 0;
    }
    git_object* raw_target = nullptr;
    if (git_object_lookup(&raw_target, r.get(), &remote_oid, GIT_OBJECT_COMMIT) != 0) {
        out_pull_log = "Lookup failed";
        finalize();
        return 2;
    }
    object_ptr target(raw_target, git_object_free);
    if (git_reset(r.get(), target.get(), GIT_RESET_HARD, nullptr) != 0) {
        out_pull_log = "Reset failed";
        finalize();
        return 2;
    }
    out_pull_log = "Fast-forwarded";
    finalize();
    return 0;
}

} // namespace git
