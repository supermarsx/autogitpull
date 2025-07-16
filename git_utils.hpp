#ifndef GIT_UTILS_HPP
#define GIT_UTILS_HPP

#include <string>
#include <filesystem>
#include <functional>

namespace git {
namespace fs = std::filesystem;

/**
 * @brief RAII helper managing global libgit2 initialization.
 *
 * Instantiate once for the lifetime of the application to ensure all libgit2
 * operations are performed after initialization and before shutdown.
 */
struct GitInitGuard {
    /**
     * @brief Construct and initialize libgit2.
     *
     * Calls `git_libgit2_init()` on creation. No other side effects.
     */
    GitInitGuard();

    /**
     * @brief Shut down libgit2 on destruction.
     *
     * Invokes `git_libgit2_shutdown()` when the guard is destroyed.
     * No further side effects.
     */
    ~GitInitGuard();
};

// The utility functions below assume libgit2 is already initialized.

/**
 * @brief Determine whether the given path is a Git repository.
 *
 * @param p Filesystem path to check.
 * @return `true` if a `.git` directory exists inside @a p.
 *
 * This check does not modify the filesystem.
 */
bool is_git_repo(const fs::path &p);

/**
 * @brief Get the commit hash pointed to by `HEAD`.
 *
 * @param repo Path to a Git repository.
 * @return 40 character hexadecimal commit hash, or empty string on error.
 *
 * Reads repository metadata without altering any files.
 */
std::string get_local_hash(const fs::path &repo);

/**
 * @brief Retrieve the currently checked out branch name.
 *
 * @param repo Path to a Git repository.
 * @return Branch name or empty string if it cannot be determined.
 *
 * Queries repository state without making changes.
 */
std::string get_current_branch(const fs::path &repo);

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
 *
 * Performs a network fetch of the remote without altering the working tree.
 */
std::string get_remote_hash(const fs::path &repo, const std::string &branch,
                            bool use_credentials = false, bool *auth_failed = nullptr);

/**
 * @brief Obtain the URL of the `origin` remote.
 *
 * @param repo Path to a Git repository.
 * @return Remote URL as a string, or empty string on failure.
 *
 * Reads repository configuration without modification.
 */
std::string get_origin_url(const fs::path &repo);

/**
 * @brief Check if a URL points to GitHub.
 *
 * @param url Remote URL string.
 * @return `true` if the URL contains `github.com`.
 *
 * Pure utility function with no side effects.
 */
bool is_github_url(const std::string &url);

/**
 * @brief Attempt to connect to the `origin` remote.
 *
 * @param repo Path to a Git repository.
 * @return `true` if the remote can be contacted.
 *
 * Performs a network probe without modifying the repository.
 */
bool remote_accessible(const fs::path &repo);

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
 *
 * May update the working tree and local references to match the remote.
 */
int try_pull(const fs::path &repo, std::string &out_pull_log,
             const std::function<void(int)> *progress_cb = nullptr, bool use_credentials = false,
             bool *auth_failed = nullptr);

} // namespace git

#endif // GIT_UTILS_HPP
