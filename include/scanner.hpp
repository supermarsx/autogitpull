#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <atomic>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <chrono>

#include "repo.hpp"

std::vector<std::filesystem::path> build_repo_list(const std::filesystem::path& root,
                                                   bool recursive,
                                                   const std::vector<std::filesystem::path>& ignore,
                                                   size_t max_depth);

void process_repo(const std::filesystem::path& p,
                  std::map<std::filesystem::path, RepoInfo>& repo_infos,
                  std::set<std::filesystem::path>& skip_repos, std::mutex& mtx,
                  std::atomic<bool>& running, std::string& action, std::mutex& action_mtx,
                  bool include_private, const std::filesystem::path& log_dir, bool check_only,
                  bool hash_check, size_t down_limit, size_t up_limit, size_t disk_limit,
                  bool silent, bool cli_mode, bool force_pull, bool skip_timeout,
                  std::chrono::seconds updated_since);

void scan_repos(const std::vector<std::filesystem::path>& all_repos,
                std::map<std::filesystem::path, RepoInfo>& repo_infos,
                std::set<std::filesystem::path>& skip_repos, std::mutex& mtx,
                std::atomic<bool>& scanning_flag, std::atomic<bool>& running, std::string& action,
                std::mutex& action_mtx, bool include_private, const std::filesystem::path& log_dir,
                bool check_only, bool hash_check, size_t concurrency, int cpu_percent_limit,
                size_t mem_limit, size_t down_limit, size_t up_limit, size_t disk_limit,
                bool silent, bool cli_mode, bool force_pull, bool skip_timeout,
                std::chrono::seconds updated_since);

#endif // SCANNER_HPP
