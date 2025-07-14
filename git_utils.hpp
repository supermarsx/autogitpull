#ifndef GIT_UTILS_HPP
#define GIT_UTILS_HPP

#include <string>
#include <filesystem>

namespace git {
namespace fs = std::filesystem;

struct CmdResult {
    int exit_code = -1;
    std::string output;
};

CmdResult run_cmd(const std::string& cmd, int timeout_sec = 0);
std::string quote_path(const fs::path& p);
bool is_git_repo(const fs::path& p);
std::string get_local_hash(const fs::path& repo);
std::string get_current_branch(const fs::path& repo);
std::string get_remote_hash(const fs::path& repo, const std::string& branch);
std::string get_origin_url(const fs::path& repo);
bool is_github_url(const std::string& url);
bool remote_accessible(const fs::path& repo);
int try_pull(const fs::path& repo, std::string& out_pull_log);

} // namespace git

#endif // GIT_UTILS_HPP
