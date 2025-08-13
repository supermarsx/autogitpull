#include "git_utils.hpp"
#include <cstdlib>
#include <chrono>
#include <functional>
#include <thread>
#include <ctime>
#include <algorithm>
#include "resource_utils.hpp"
#include "options.hpp"

using namespace std;

namespace git {

static unsigned int g_libgit_timeout = 0;

void set_libgit_timeout(unsigned int seconds) {
    g_libgit_timeout = seconds;
#ifdef GIT_OPT_SET_TIMEOUT
    if (g_libgit_timeout > 0)
        git_libgit2_opts(GIT_OPT_SET_TIMEOUT, g_libgit_timeout);
#else
    (void)g_libgit_timeout;
#endif
}

struct ProgressData {
    const std::function<void(int)>* cb;
    std::chrono::steady_clock::time_point start;
    size_t down_limit;
    size_t up_limit;
    size_t disk_limit;
};

int credential_cb(git_credential** out, const char* url, const char* username_from_url,
                  unsigned int allowed_types, void* payload) {
    const Options* opts = static_cast<const Options*>(payload);
    const char* env_user = getenv("GIT_USERNAME");
    const char* env_pass = getenv("GIT_PASSWORD");
    const char* user = username_from_url ? username_from_url : env_user;
    if ((allowed_types & GIT_CREDENTIAL_SSH_KEY) && opts && !opts->ssh_private_key.empty() &&
        user) {
        const char* pub =
            opts->ssh_public_key.empty() ? nullptr : opts->ssh_public_key.string().c_str();
        if (git_credential_ssh_key_new(out, user, pub, opts->ssh_private_key.string().c_str(),
                                       "") == 0)
            return 0;
    }
    if ((allowed_types & GIT_CREDENTIAL_SSH_KEY) && user) {
        if (git_credential_ssh_key_from_agent(out, user) == 0)
            return 0;
    }
    if ((allowed_types & GIT_CREDENTIAL_USERNAME) && user) {
        if (git_credential_username_new(out, user) == 0)
            return 0;
    }
    if ((allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) && env_user && env_pass)
        return git_credential_userpass_plaintext_new(out, env_user, env_pass);
    return git_credential_default_new(out);
}

GitInitGuard::GitInitGuard() {
    git_libgit2_init();
#ifdef GIT_OPT_SET_TIMEOUT
    if (g_libgit_timeout > 0)
        git_libgit2_opts(GIT_OPT_SET_TIMEOUT, g_libgit_timeout);
#endif
}

GitInitGuard::~GitInitGuard() { git_libgit2_shutdown(); }

bool is_git_repo(const fs::path& p) {
    return fs::exists(p / ".git") && fs::is_directory(p / ".git");
}

static string oid_to_hex(const git_oid& oid) {
    char buf[GIT_OID_HEXSZ + 1];
    git_oid_tostr(buf, sizeof(buf), &oid);
    return string(buf);
}

static void set_error(std::string* error) {
    if (!error)
        return;
    const git_error* e = git_error_last();
    if (e && e->message)
        *error = e->message;
    else
        *error = "Unknown libgit2 error";
}

optional<string> get_local_hash(const fs::path& repo, string* error) {
    git_repository* raw = nullptr;
    if (git_repository_open(&raw, repo.string().c_str()) != 0) {
        set_error(error);
        return nullopt;
    }
    repo_ptr r(raw);
    git_oid oid;
    if (git_reference_name_to_id(&oid, r.get(), "HEAD") != 0) {
        set_error(error);
        return nullopt;
    }
    return oid_to_hex(oid);
}

optional<string> get_current_branch(const fs::path& repo, string* error) {
    git_repository* raw = nullptr;
    if (git_repository_open(&raw, repo.string().c_str()) != 0) {
        set_error(error);
        return nullopt;
    }
    repo_ptr r(raw);
    git_reference* head = nullptr;
    if (git_repository_head(&head, r.get()) != 0) {
        set_error(error);
        return nullopt;
    }
    reference_ptr ref(head);
    const char* name = git_reference_shorthand(ref.get());
    string branch = name ? name : "";
    if (branch.empty()) {
        set_error(error);
        return nullopt;
    }
    return branch;
}

static git_repository* open_and_fetch_remote(const fs::path& repo, const string& remote,
                                             bool use_credentials, bool* auth_failed,
                                             string* error) {
    git_repository* raw_repo = nullptr;
    if (git_repository_open(&raw_repo, repo.string().c_str()) != 0) {
        set_error(error);
        return nullptr;
    }
    git_remote* raw_remote = nullptr;
    if (git_remote_lookup(&raw_remote, raw_repo, remote.c_str()) != 0) {
        set_error(error);
        git_repository_free(raw_repo);
        return nullptr;
    }
    remote_ptr remote_handle(raw_remote);
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    if (use_credentials)
        fetch_opts.callbacks.credentials = credential_cb;
    int err = git_remote_fetch(remote_handle.get(), nullptr, &fetch_opts, nullptr);
    if (err != 0) {
        const git_error* e = git_error_last();
        if (auth_failed && e && e->message &&
            std::string(e->message).find("auth") != std::string::npos)
            *auth_failed = true;
        set_error(error);
    }
    return raw_repo;
}

optional<string> get_remote_hash(const fs::path& repo, const string& remote, const string& branch,
                                 bool use_credentials, bool* auth_failed, string* error) {
    git_repository* raw_repo =
        open_and_fetch_remote(repo, remote, use_credentials, auth_failed, error);
    if (!raw_repo)
        return nullopt;
    repo_ptr r(raw_repo);
    git_oid oid;
    string refname = string("refs/remotes/") + remote + "/" + branch;
    if (git_reference_name_to_id(&oid, r.get(), refname.c_str()) != 0) {
        set_error(error);
        return nullopt;
    }
    return oid_to_hex(oid);
}

std::time_t get_remote_commit_time(const fs::path& repo, const std::string& remote,
                                   const std::string& branch, bool use_credentials,
                                   bool* auth_failed) {
    git_repository* raw_repo =
        open_and_fetch_remote(repo, remote, use_credentials, auth_failed, nullptr);
    if (!raw_repo)
        return 0;
    repo_ptr r(raw_repo);
    git_oid oid;
    std::string refname = std::string("refs/remotes/") + remote + "/" + branch;
    if (git_reference_name_to_id(&oid, r.get(), refname.c_str()) != 0)
        return 0;
    git_commit* commit = nullptr;
    if (git_commit_lookup(&commit, r.get(), &oid) != 0)
        return 0;
    object_ptr cmt(reinterpret_cast<git_object*>(commit));
    git_time_t t = git_commit_time(commit);
    return static_cast<std::time_t>(t);
}

optional<string> get_remote_url(const fs::path& repo, const string& remote, string* error) {
    git_repository* raw_repo = nullptr;
    if (git_repository_open(&raw_repo, repo.string().c_str()) != 0) {
        set_error(error);
        return nullopt;
    }
    repo_ptr r(raw_repo);
    git_remote* raw_remote = nullptr;
    if (git_remote_lookup(&raw_remote, r.get(), remote.c_str()) != 0) {
        set_error(error);
        return nullopt;
    }
    remote_ptr remote_handle(raw_remote);
    const char* url = git_remote_url(remote_handle.get());
    if (!url) {
        set_error(error);
        return nullopt;
    }
    return string(url);
}

bool is_github_url(const string& url) { return url.find("github.com") != string::npos; }

bool remote_accessible(const fs::path& repo, const string& remote) {
    git_repository* raw_repo = nullptr;
    if (git_repository_open(&raw_repo, repo.string().c_str()) != 0)
        return false;
    repo_ptr r(raw_repo);
    git_remote* raw_remote = nullptr;
    if (git_remote_lookup(&raw_remote, r.get(), remote.c_str()) != 0)
        return false;
    remote_ptr remote_handle(raw_remote);
    int err = git_remote_connect(remote_handle.get(), GIT_DIRECTION_FETCH, nullptr, nullptr, nullptr);
    bool ok = err == 0;
    if (ok)
        git_remote_disconnect(remote_handle.get());
    return ok;
}

bool has_uncommitted_changes(const fs::path& repo) {
    git_repository* raw_repo = nullptr;
    if (git_repository_open(&raw_repo, repo.string().c_str()) != 0)
        return false;
    repo_ptr r(raw_repo);
    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;
    git_status_list* raw_list = nullptr;
    if (git_status_list_new(&raw_list, r.get(), &opts) != 0)
        return false;
    status_list_ptr list(raw_list);
    return git_status_list_entrycount(list.get()) > 0;
}

bool clone_repo(const fs::path& dest, const std::string& url, bool use_credentials,
                bool* auth_failed) {
    git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    ProgressData progress{nullptr, std::chrono::steady_clock::now(), 0, 0, 0};
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
            auto net = procutil::get_network_usage();
            double ms = (double)net.upload_bytes / (pd->up_limit * 1024.0) * 1000.0;
            if (ms > expected_ms)
                expected_ms = ms;
        }
        if (pd->disk_limit > 0) {
            auto du = procutil::get_disk_usage();
            double ms =
                (double)(du.read_bytes + du.write_bytes) / (pd->disk_limit * 1024.0) * 1000.0;
            if (ms > expected_ms)
                expected_ms = ms;
        }
        if (expected_ms > elapsed) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(expected_ms - elapsed)));
        }
        return 0;
    };
    if (use_credentials)
        callbacks.credentials = credential_cb;
    opts.fetch_opts.callbacks = callbacks;
    git_repository* raw_repo = nullptr;
    int err = git_clone(&raw_repo, url.c_str(), dest.string().c_str(), &opts);
    if (err != 0) {
        const git_error* e = git_error_last();
        if (auth_failed && e && e->message &&
            std::string(e->message).find("auth") != std::string::npos)
            *auth_failed = true;
        return false;
    }
    repo_ptr repo(raw_repo);
    return true;
}

int try_pull(const fs::path& repo, const string& remote_name, string& out_pull_log,
             const std::function<void(int)>* progress_cb, bool use_credentials, bool* auth_failed,
             size_t down_limit_kbps, size_t up_limit_kbps, size_t disk_limit_kbps,
             bool force_pull) {

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
    repo_ptr r(raw_repo);
    string branch;
    {
        string err;
        auto br = get_current_branch(repo, &err);
        if (!br) {
            out_pull_log = err;
            finalize();
            return 2;
        }
        branch = *br;
    }
    git_remote* raw_remote = nullptr;
    if (git_remote_lookup(&raw_remote, r.get(), remote_name.c_str()) != 0) {
        out_pull_log = "No " + remote_name + " remote";
        finalize();
        return 2;
    }
    remote_ptr remote_handle(raw_remote);
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    ProgressData progress{progress_cb, std::chrono::steady_clock::now(), down_limit_kbps,
                          up_limit_kbps, disk_limit_kbps};
    if (up_limit_kbps > 0)
        procutil::init_network_usage();
    if (progress_cb || down_limit_kbps > 0 || up_limit_kbps > 0 || disk_limit_kbps > 0) {
        if (disk_limit_kbps > 0)
            procutil::init_disk_usage();
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
                auto net = procutil::get_network_usage();
                double ms = (double)net.upload_bytes / (pd->up_limit * 1024.0) * 1000.0;
                if (ms > expected_ms)
                    expected_ms = ms;
            }
            if (pd->disk_limit > 0) {
                auto du = procutil::get_disk_usage();
                double ms =
                    (double)(du.read_bytes + du.write_bytes) / (pd->disk_limit * 1024.0) * 1000.0;
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
    int err = git_remote_fetch(remote_handle.get(), nullptr, &fetch_opts, nullptr);
    if (err != 0) {
        const git_error* e = git_error_last();
        if (auth_failed && e && e->message &&
            std::string(e->message).find("auth") != std::string::npos)
            *auth_failed = true;
        std::string msg = "Fetch failed";
        if (e && e->message)
            msg = e->message;
        std::string msg_lower = msg;
        std::transform(msg_lower.begin(), msg_lower.end(), msg_lower.begin(), ::tolower);
        bool is_rate_limit = msg_lower.find("rate limit") != std::string::npos ||
                             msg_lower.find("429") != std::string::npos;
        if (is_rate_limit) {
            finalize();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            err = git_remote_fetch(remote_handle.get(), nullptr, &fetch_opts, nullptr);
            if (err != 0) {
                e = git_error_last();
                msg = "Fetch failed";
                if (e && e->message)
                    msg = e->message;
                msg_lower = msg;
                std::transform(msg_lower.begin(), msg_lower.end(), msg_lower.begin(), ::tolower);
                out_pull_log = msg;
                finalize();
                if (msg_lower.find("timed out") != std::string::npos ||
                    msg_lower.find("timeout") != std::string::npos)
                    return TRY_PULL_TIMEOUT;
                if (msg_lower.find("rate limit") != std::string::npos ||
                    msg_lower.find("429") != std::string::npos)
                    return TRY_PULL_RATE_LIMIT;
                return 2;
            }
        } else {
            out_pull_log = msg;
            finalize();
            if (msg_lower.find("timed out") != std::string::npos ||
                msg_lower.find("timeout") != std::string::npos)
                return TRY_PULL_TIMEOUT;
            if (msg_lower.find("rate limit") != std::string::npos ||
                msg_lower.find("429") != std::string::npos)
                return TRY_PULL_RATE_LIMIT;
            return 2;
        }
    }
    git_oid remote_oid;
    string refname = string("refs/remotes/") + remote_name + "/" + branch;
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
    if (!force_pull && has_uncommitted_changes(repo)) {
        out_pull_log = "Local changes present";
        finalize();
        return 3;
    }
    git_object* raw_target = nullptr;
    if (git_object_lookup(&raw_target, r.get(), &remote_oid, GIT_OBJECT_COMMIT) != 0) {
        out_pull_log = "Lookup failed";
        finalize();
        return 2;
    }
    object_ptr target(raw_target);
    if (git_reset(r.get(), target.get(), GIT_RESET_HARD, nullptr) != 0) {
        out_pull_log = "Reset failed";
        finalize();
        return 2;
    }
    out_pull_log = "Fast-forwarded";
    finalize();
    return 0;
}

string get_last_commit_date(const fs::path& repo) {
    git_repository* raw = nullptr;
    if (git_repository_open(&raw, repo.string().c_str()) != 0)
        return "";
    repo_ptr r(raw);
    git_oid oid;
    if (git_reference_name_to_id(&oid, r.get(), "HEAD") != 0)
        return "";
    git_commit* commit = nullptr;
    if (git_commit_lookup(&commit, r.get(), &oid) != 0)
        return "";
    object_ptr cmt(reinterpret_cast<git_object*>(commit));
    git_time_t t = git_commit_time(commit);
    std::time_t tt = static_cast<std::time_t>(t);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return string(buf);
}

std::time_t get_last_commit_time(const fs::path& repo) {
    git_repository* raw = nullptr;
    if (git_repository_open(&raw, repo.string().c_str()) != 0)
        return 0;
    repo_ptr r(raw);
    git_oid oid;
    if (git_reference_name_to_id(&oid, r.get(), "HEAD") != 0)
        return 0;
    git_commit* commit = nullptr;
    if (git_commit_lookup(&commit, r.get(), &oid) != 0)
        return 0;
    object_ptr cmt(reinterpret_cast<git_object*>(commit));
    git_time_t t = git_commit_time(commit);
    return static_cast<std::time_t>(t);
}

string get_last_commit_author(const fs::path& repo) {
    git_repository* raw = nullptr;
    if (git_repository_open(&raw, repo.string().c_str()) != 0)
        return "";
    repo_ptr r(raw);
    git_oid oid;
    if (git_reference_name_to_id(&oid, r.get(), "HEAD") != 0)
        return "";
    git_commit* commit = nullptr;
    if (git_commit_lookup(&commit, r.get(), &oid) != 0)
        return "";
    object_ptr cmt(reinterpret_cast<git_object*>(commit));
    const git_signature* sig = git_commit_author(commit);
    if (!sig || !sig->name)
        return "";
    return string(sig->name);
}

} // namespace git
