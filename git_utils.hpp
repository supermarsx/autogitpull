#ifndef GIT_UTILS_HPP
#define GIT_UTILS_HPP

#include <string>
#include <filesystem>
#include <functional>

namespace git {
namespace fs = std::filesystem;

// RAII helper that manages global libgit2 initialization.
// Instantiate once (e.g. in main) to keep the library active for
// the lifetime of all git utility calls.
struct GitInitGuard {
    GitInitGuard();
    ~GitInitGuard();
};

// The utility functions below do not perform libgit2 initialization
// themselves. Ensure a GitInitGuard exists while they are in use.

bool is_git_repo(const fs::path& p);
std::string get_local_hash(const fs::path& repo);
std::string get_current_branch(const fs::path& repo);
std::string get_remote_hash(const fs::path& repo, const std::string& branch,
                            bool use_credentials = false,
                            bool* auth_failed = nullptr);
std::string get_origin_url(const fs::path& repo);
bool is_github_url(const std::string& url);
bool remote_accessible(const fs::path& repo);
int try_pull(const fs::path& repo, std::string& out_pull_log,
             const std::function<void(int)>* progress_cb = nullptr,
             bool use_credentials = false, bool* auth_failed = nullptr);

} // namespace git

#endif // GIT_UTILS_HPP
