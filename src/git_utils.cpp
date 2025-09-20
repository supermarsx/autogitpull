#include "git_utils.hpp"
#include <cstdlib>
#include <chrono>
#include <functional>
#include <fstream>
#include <thread>
#include <ctime>
#include <algorithm>
#include "resource_utils.hpp"
#include "options.hpp"

using namespace std;

namespace git {

static unsigned int g_libgit_timeout = 0;

/**
 * @brief Read credentials from a file.
 *
 * The file is expected to contain the username on the first line and the
 * password on the second line.
 *
 * @param path Path to the credential file.
 * @param user Output parameter receiving the username.
 * @param pass Output parameter receiving the password.
 * @return True if both username and password were read.
 */
static bool read_credential_file(const fs::path& path, std::string& user, std::string& pass) {
    std::ifstream ifs(path);
    if (!ifs)
        return false;
    std::getline(ifs, user);
    std::getline(ifs, pass);
    return !user.empty() && !pass.empty();
}

/**
 * @brief Configure the global libgit2 network timeout.
 *
 * When supported by the linked libgit2, this sets a timeout applied to
 * network operations initiated by the library.
 *
 * @param seconds Timeout value in seconds.
 * @return None.
 */
void set_libgit_timeout(unsigned int seconds) {
    g_libgit_timeout = seconds;
#ifdef GIT_OPT_SET_TIMEOUT
    if (g_libgit_timeout > 0)
        git_libgit2_opts(GIT_OPT_SET_TIMEOUT, g_libgit_timeout);
#else
    (void)g_libgit_timeout;
#endif
}

void set_proxy(const std::string& url) {
#ifdef GIT_OPT_SET_PROXY
    git_libgit2_opts(GIT_OPT_SET_PROXY, url.empty() ? nullptr : url.c_str());
#else
    (void)url;
#endif
}

struct ProgressData {
    const std::function<void(int)>* cb;
    std::chrono::steady_clock::time_point start;
    size_t down_limit;
    size_t up_limit;
    size_t disk_limit;
};

/**
 * @brief Build a transfer progress callback with rate limiting support.
 *
 * The callback reports percentage progress and throttles network and disk
 * usage according to the provided limits.
 *
 * @param cb Optional user callback receiving progress percentage.
 * @param pd Structure tracking rate limits and start time.
 * @return libgit2 transfer progress callback or `nullptr` if unused.
 */
static git_transfer_progress_cb make_progress_callback(const std::function<void(int)>* cb,
                                                       ProgressData& pd) {
    pd.cb = cb;
    if (!cb && pd.down_limit == 0 && pd.up_limit == 0 && pd.disk_limit == 0)
        return nullptr;
    return [](const git_transfer_progress* stats, void* payload) -> int {
        if (!payload)
            return 0;
        auto* pd = static_cast<ProgressData*>(payload);
        if (pd->cb) {
            int pct = 0;
            if (stats->total_objects > 0)
                pct = static_cast<int>(100 * stats->received_objects / stats->total_objects);
            // Report progress percentage to the user callback
            (*pd->cb)(pct);
        }
        double expected_ms = 0.0;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - pd->start)
                           .count();
        if (pd->down_limit > 0) {
            double ms =
                static_cast<double>(stats->received_bytes) / (pd->down_limit * 1024.0) * 1000.0;
            if (ms > expected_ms)
                expected_ms = ms; // enforce download rate limit
        }
        if (pd->up_limit > 0) {
            auto net = procutil::get_network_usage();
            double ms = static_cast<double>(net.upload_bytes) / (pd->up_limit * 1024.0) * 1000.0;
            if (ms > expected_ms)
                expected_ms = ms; // enforce upload rate limit
        }
        if (pd->disk_limit > 0) {
            auto du = procutil::get_disk_usage();
            double ms = static_cast<double>(du.read_bytes + du.write_bytes) /
                        (pd->disk_limit * 1024.0) * 1000.0;
            if (ms > expected_ms)
                expected_ms = ms; // enforce disk I/O rate limit
        }
        if (expected_ms > elapsed) {
            // Sleep to throttle throughput when exceeding rate limits
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(expected_ms - elapsed)));
        }
        return 0;
    };
}

/**
 * @brief libgit2 credential callback implementing precedence rules.
 *
 * Credentials are chosen in the following order:
 *  1. Explicit SSH key provided via options.
 *  2. Username only.
 *  3. SSH agent.
 *  4. Username/password from file.
 *  5. Username/password from environment variables.
 *  6. Default credential helper.
 *
 * @param out             Output credential pointer for libgit2.
 * @param url             Remote URL being accessed.
 * @param username_from_url Username parsed from the URL, if any.
 * @param allowed_types   Bitmask of credential types libgit2 will accept.
 * @param payload         Pointer to Options providing credential data.
 * @return 0 on success or a libgit2 error code.
 */
static std::optional<std::string> safe_getenv(const char* name) {
#ifdef _WIN32
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, name) == 0 && buf) {
        std::string v(buf);
        free(buf);
        return v;
    }
    return std::nullopt;
#else
    const char* v = std::getenv(name);
    if (v)
        return std::string(v);
    return std::nullopt;
#endif
}

int credential_cb(git_credential** out, const char* url, const char* username_from_url,
                  unsigned int allowed_types, void* payload) {
    GitInitGuard guard;
    (void)url;
    const Options* opts = static_cast<const Options*>(payload);
    auto env_user = safe_getenv("GIT_USERNAME");
    auto env_pass = safe_getenv("GIT_PASSWORD");
    std::string file_user;
    std::string file_pass;
    if (opts && !opts->credential_file.empty())
        read_credential_file(opts->credential_file, file_user,
                             file_pass); // load file credentials when provided
    const char* user =
        username_from_url
            ? username_from_url
            : (!file_user.empty() ? file_user.c_str() : (env_user ? env_user->c_str() : nullptr));
    if ((allowed_types & GIT_CREDENTIAL_SSH_KEY) && opts && !opts->ssh_private_key.empty() &&
        user) {
        const char* pub = nullptr;
        std::string pub_buf;
        if (!opts->ssh_public_key.empty()) {
            pub_buf = opts->ssh_public_key.string();
            pub = pub_buf.c_str();
        }
        if (git_credential_ssh_key_new(out, user, pub, opts->ssh_private_key.string().c_str(),
                                       "") == 0)
            return 0; // prefer explicit SSH key
    }
    if ((allowed_types & GIT_CREDENTIAL_USERNAME) && user) {
        if (git_credential_username_new(out, user) == 0)
            return 0; // fallback to username-only auth
    }
    if ((allowed_types & GIT_CREDENTIAL_SSH_KEY) && user) {
        if (git_credential_ssh_key_from_agent(out, user) == 0)
            return 0; // try SSH agent next
    }
    if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
        if (!file_user.empty() && !file_pass.empty())
            return git_credential_userpass_plaintext_new(
                out, file_user.c_str(),
                file_pass.c_str()); // file credentials take precedence over env
        if (env_user && env_pass)
            return git_credential_userpass_plaintext_new(
                out, env_user->c_str(), env_pass->c_str()); // finally, use environment
    }
    return git_credential_default_new(out); // fall back to system credential helper
}

/**
 * @brief Construct the RAII guard and initialize libgit2.
 * @return None.
 */
GitInitGuard::GitInitGuard() {
    git_libgit2_init();
#ifdef GIT_OPT_SET_TIMEOUT
    if (g_libgit_timeout > 0)
        git_libgit2_opts(GIT_OPT_SET_TIMEOUT, g_libgit_timeout);
#endif
}

/**
 * @brief Destroy the RAII guard and shutdown libgit2.
 * @return None.
 */
GitInitGuard::~GitInitGuard() { git_libgit2_shutdown(); }

/**
 * @brief Determine whether a path is a Git repository.
 *
 * @param p Filesystem path to inspect.
 * @return True if a `.git` directory exists.
 */
bool is_git_repo(const fs::path& p) {
    return fs::exists(p / ".git") && fs::is_directory(p / ".git");
}

/**
 * @brief Convert a libgit2 object ID to a hexadecimal string.
 *
 * @param oid Object identifier to convert.
 * @return Hexadecimal representation of @a oid.
 */
static string oid_to_hex(const git_oid& oid) {
    char buf[GIT_OID_HEXSZ + 1];
    git_oid_tostr(buf, sizeof(buf), &oid);
    return string(buf);
}

/**
 * @brief Populate an error string with the last libgit2 error message.
 *
 * @param error Output string receiving the error description.
 * @return None.
 */
static void set_error(std::string* error) {
    if (!error)
        return;
    const git_error* e = git_error_last();
    if (e && e->message)
        *error = e->message;
    else
        *error = "Unknown libgit2 error";
}

/**
 * @brief Obtain the commit hash pointed to by HEAD.
 *
 * @param repo  Path to an existing repository.
 * @param error Optional output string receiving an error description.
 * @return Hash string on success or `std::nullopt` on failure.
 */
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

/**
 * @brief Determine the currently checked-out branch.
 *
 * @param repo  Path to an existing repository.
 * @param error Optional output string receiving an error description.
 * @return Branch name or `std::nullopt` if it cannot be determined.
 */
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

/**
 * @brief Open a repository and fetch a remote.
 *
 * @param repo           Path to repository.
 * @param remote         Name of remote to fetch.
 * @param use_credentials Whether to use credential callback.
 * @param auth_failed    Output flag set when authentication fails.
 * @param error          Optional output string for error details.
 * @return Pointer to opened repository or `nullptr` on failure.
 */
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
        fetch_opts.callbacks.credentials = credential_cb; // provide credentials when requested
    int err = git_remote_fetch(remote_handle.get(), nullptr, &fetch_opts, nullptr);
    if (err != 0) {
        const git_error* e = git_error_last();
        if (auth_failed && e && e->message &&
            std::string(e->message).find("auth") != std::string::npos)
            *auth_failed = true;
        git_repository_free(raw_repo);
        set_error(error);
        return nullptr;
    }
    return raw_repo;
}

/**
 * @brief Fetch remote branch and return its commit hash.
 *
 * @param repo           Path to repository.
 * @param remote         Name of remote.
 * @param branch         Branch name on remote.
 * @param use_credentials Whether to use credential callback.
 * @param auth_failed    Optional output flag set on auth failure.
 * @param error          Optional output string receiving error details.
 * @return Remote commit hash or `std::nullopt` on error.
 */
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

/**
 * @brief Retrieve the commit time of a remote branch.
 *
 * @param repo           Path to repository.
 * @param remote         Name of remote.
 * @param branch         Branch name on remote.
 * @param use_credentials Whether to use credential callback.
 * @param auth_failed    Optional output flag set on auth failure.
 * @return Commit timestamp or 0 on error.
 */
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

/**
 * @brief Retrieve the configured URL for a remote.
 *
 * @param repo  Path to repository.
 * @param remote Remote name.
 * @param error Optional output string receiving error details.
 * @return Remote URL or `std::nullopt` on error.
 */
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

/**
 * @brief Check whether a URL targets GitHub.
 *
 * @param url URL string to test.
 * @return True if the URL contains "github.com".
 */
bool is_github_url(const string& url) { return url.find("github.com") != string::npos; }

/**
 * @brief Test connectivity to a remote.
 *
 * @param repo   Path to repository.
 * @param remote Remote name to check.
 * @return True if the remote can be connected to.
 */
bool remote_accessible(const fs::path& repo, const string& remote) {
    git_repository* raw_repo = nullptr;
    if (git_repository_open(&raw_repo, repo.string().c_str()) != 0)
        return false;
    repo_ptr r(raw_repo);
    git_remote* raw_remote = nullptr;
    if (git_remote_lookup(&raw_remote, r.get(), remote.c_str()) != 0)
        return false;
    remote_ptr remote_handle(raw_remote);
    int err =
        git_remote_connect(remote_handle.get(), GIT_DIRECTION_FETCH, nullptr, nullptr, nullptr);
    bool ok = err == 0;
    if (ok)
        git_remote_disconnect(remote_handle.get());
    return ok;
}

/**
 * @brief Determine whether the repository has uncommitted changes.
 *
 * @param repo Path to repository.
 * @return True if the working tree is dirty.
 */
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

/**
 * @brief Clone a repository while enforcing optional rate limits.
 *
 * @param dest            Destination path for clone.
 * @param url             Remote URL.
 * @param progress_cb     Optional callback receiving progress percent.
 * @param use_credentials Whether to use credential callback.
 * @param auth_failed     Output flag set if authentication fails.
 * @param down_limit_kbps Download rate limit in KiB/s.
 * @param up_limit_kbps   Upload rate limit in KiB/s.
 * @param disk_limit_kbps Disk I/O rate limit in KiB/s.
 * @return True on success, false otherwise.
 */
bool clone_repo(const fs::path& dest, const std::string& url,
                const std::function<void(int)>* progress_cb, bool use_credentials,
                bool* auth_failed, size_t down_limit_kbps, size_t up_limit_kbps,
                size_t disk_limit_kbps) {
    git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    ProgressData progress{nullptr, std::chrono::steady_clock::now(), down_limit_kbps, up_limit_kbps,
                          disk_limit_kbps};
    if (up_limit_kbps > 0)
        procutil::init_network_usage(); // track upload usage for rate limiting
    auto transfer_cb = make_progress_callback(progress_cb, progress);
    if (transfer_cb) {
        if (disk_limit_kbps > 0)
            procutil::init_disk_usage(); // track disk I/O for throttling
        callbacks.payload = &progress;
        callbacks.transfer_progress = transfer_cb; // enable progress reporting and limits
    }
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

/**
 * @brief Fast-forward pull from a remote with retry on rate limiting.
 *
 * @param repo             Path to repository.
 * @param remote_name      Name of remote.
 * @param out_pull_log     Receives textual description of the action.
 * @param progress_cb      Optional progress callback.
 * @param use_credentials  Whether to use credential callback.
 * @param auth_failed      Output flag set if authentication fails.
 * @param down_limit_kbps  Download rate limit in KiB/s.
 * @param up_limit_kbps    Upload rate limit in KiB/s.
 * @param disk_limit_kbps  Disk I/O rate limit in KiB/s.
 * @param force_pull       Ignore local changes and force reset.
 * @return Status code: 0 success, 2 failure, TRY_PULL_* constants for
 *         specific error cases.
 */
int try_pull(const fs::path& repo, const string& remote_name, string& out_pull_log,
             const std::function<void(int)>* progress_cb, bool use_credentials, bool* auth_failed,
             size_t down_limit_kbps, size_t up_limit_kbps, size_t disk_limit_kbps,
             bool force_pull) {

    if (progress_cb)
        (*progress_cb)(0); // begin progress reporting
    auto finalize = [&]() {
        if (progress_cb)
            (*progress_cb)(100); // ensure 100% on completion
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
    ProgressData progress{nullptr, std::chrono::steady_clock::now(), down_limit_kbps, up_limit_kbps,
                          disk_limit_kbps};
    if (up_limit_kbps > 0)
        procutil::init_network_usage(); // capture initial network usage for rate limiting
    auto transfer_cb = make_progress_callback(progress_cb, progress);
    if (transfer_cb) {
        if (disk_limit_kbps > 0)
            procutil::init_disk_usage(); // start tracking disk I/O
        callbacks.payload = &progress;
        callbacks.transfer_progress = transfer_cb; // enable progress and throttling
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
        std::transform(msg_lower.begin(), msg_lower.end(), msg_lower.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        bool is_rate_limit = msg_lower.find("rate limit") != std::string::npos ||
                             msg_lower.find("429") != std::string::npos;
        if (is_rate_limit) {
            finalize();
            std::this_thread::sleep_for(std::chrono::seconds(2)); // back off before retrying
            err = git_remote_fetch(remote_handle.get(), nullptr, &fetch_opts, nullptr);
            if (err != 0) {
                e = git_error_last();
                msg = "Fetch failed";
                if (e && e->message)
                    msg = e->message;
                msg_lower = msg;
                std::transform(msg_lower.begin(), msg_lower.end(), msg_lower.begin(),
                               [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
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

/**
 * @brief Return formatted date of the last commit.
 *
 * @param repo Path to repository.
 * @return Timestamp string or empty string on error.
 */
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

/**
 * @brief Obtain the timestamp of the last commit.
 *
 * @param repo Path to repository.
 * @return time_t of last commit or 0 on error.
 */
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

/**
 * @brief Retrieve the author name of the last commit.
 *
 * @param repo Path to repository.
 * @return Author name or empty string on error.
 */
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
