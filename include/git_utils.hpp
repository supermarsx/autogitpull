#ifndef GIT_UTILS_HPP
#define GIT_UTILS_HPP

#include <git2.h>
#include <string>
#include <filesystem>
#include <functional>
#include <memory>

namespace git {
namespace fs = std::filesystem;

/**
 * @brief RAII helper managing global libgit2 initialization.
 *
 * Instantiate once for the lifetime of the application to ensure all libgit2
 * operations are performed after initialization and before shutdown.
 */
struct GitInitGuard {
    GitInitGuard();  ///< Calls `git_libgit2_init()`
    ~GitInitGuard(); ///< Calls `git_libgit2_shutdown()`
};

void set_libgit_timeout(unsigned int seconds);

// RAII wrappers for libgit2 resources
template <typename T, void (*Free)(T*)> struct GitHandle {
    T* h;
    explicit GitHandle(T* h_ = nullptr) : h(h_) {}
    ~GitHandle() {
        if (h)
            Free(h);
    }
    T* get() const { return h; }
};

using repo_ptr = GitHandle<git_repository, git_repository_free>;
using remote_ptr = GitHandle<git_remote, git_remote_free>;
using object_ptr = GitHandle<git_object, git_object_free>;
using reference_ptr = GitHandle<git_reference, git_reference_free>;
using status_list_ptr = GitHandle<git_status_list, git_status_list_free>;

// The utility functions below assume libgit2 is already initialized.

/**
 * @brief Determine whether the given path is a Git repository.
 *
 * @param p Filesystem path to check.
 * @return `true` if a `.git` directory exists inside @a p.
 */
bool is_git_repo(const fs::path& p);

/**
 * @brief Get the commit hash pointed to by `HEAD`.
 *
 * @param repo Path to a Git repository.
 * @return 40 character hexadecimal commit hash, or empty string on error.
 */
std::string get_local_hash(const fs::path& repo);

/**
 * @brief Retrieve the currently checked out branch name.
 *
 * @param repo Path to a Git repository.
 * @return Branch name or empty string if it cannot be determined.
 */
std::string get_current_branch(const fs::path& repo);

/**
 * @brief Fetch `origin` and return the hash of the specified remote branch.
 *
 * @param repo           Path to a Git repository.
 * @param branch         Branch name to query on the remote.
 * @param use_credentials Whether to attempt authentication using environment
 *                        variables `GIT_USERNAME`/`GIT_PASSWORD`.
 * @param auth_failed    Optional output flag set to `true` when authentication
 *                        fails.
 * @return Commit hash of the remote branch, or empty string on failure.
 */
std::string get_remote_hash(const fs::path& repo, const std::string& branch,
                            bool use_credentials = false, bool* auth_failed = nullptr);

/**
 * @brief Obtain the URL of the `origin` remote.
 *
 * @param repo Path to a Git repository.
 * @return Remote URL as a string, or empty string on failure.
 */
std::string get_origin_url(const fs::path& repo);

/**
 * @brief Check if a URL points to GitHub.
 *
 * @param url Remote URL string.
 * @return `true` if the URL contains `github.com`.
 */
bool is_github_url(const std::string& url);

/**
 * @brief Attempt to connect to the `origin` remote.
 *
 * @param repo Path to a Git repository.
 * @return `true` if the remote can be contacted.
 */
bool remote_accessible(const fs::path& repo);

/**
 * @brief Check if there are uncommitted changes in the repository.
 */
bool has_uncommitted_changes(const fs::path& repo);

/**
 * @brief Perform a fast-forward pull from the `origin` remote.
 *
 * `try_pull` fetches the current branch from `origin` and resets the local
 * repository if the remote contains new commits. A progress callback may be
 * provided to monitor fetch progress.
 *
 * @param repo           Path to a Git repository.
 * @param out_pull_log   Receives a short textual log describing the action.
 * @param progress_cb    Optional callback reporting fetch progress in percent.
 * @param use_credentials Whether to attempt authentication using environment
 *                        variables.
 * @param auth_failed    Optional output flag set when authentication fails.
 * @return `0` on success or when already up to date, `2` on failure.
 */
int try_pull(const fs::path& repo, std::string& out_pull_log,
             const std::function<void(int)>* progress_cb = nullptr, bool use_credentials = false,
             bool* auth_failed = nullptr, size_t down_limit_kbps = 0, size_t up_limit_kbps = 0,
             size_t disk_limit_kbps = 0, bool force_pull = false);

constexpr int TRY_PULL_TIMEOUT = 4;

std::string get_last_commit_date(const fs::path& repo);
std::string get_last_commit_author(const fs::path& repo);
std::time_t get_last_commit_time(const fs::path& repo);

} // namespace git

#endif // GIT_UTILS_HPP
