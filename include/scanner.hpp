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
#include <optional>

#include "repo.hpp"
#include "repo_options.hpp"

std::vector<std::filesystem::path> build_repo_list(const std::vector<std::filesystem::path>& roots,
                                                   bool recursive,
                                                   const std::vector<std::filesystem::path>& ignore,
                                                   size_t max_depth);

void process_repo(const std::filesystem::path& p,
                  std::map<std::filesystem::path, RepoInfo>& repo_infos,
                  std::set<std::filesystem::path>& skip_repos, std::mutex& mtx,
                  std::atomic<bool>& running, std::string& action, std::mutex& action_mtx,
                  bool include_private, const std::string& remote,
                  const std::filesystem::path& log_dir, bool check_only, bool hash_check,
                  size_t down_limit, size_t up_limit, size_t disk_limit, bool silent, bool cli_mode,
                  bool dry_run, bool force_pull, bool skip_timeout, bool skip_unavailable,
                  bool skip_accessible_errors, const std::filesystem::path& post_pull_hook,
                  const std::optional<std::string>& pull_ref, std::chrono::seconds updated_since,
                  bool show_pull_author, std::chrono::seconds pull_timeout, bool mutant_mode);

void scan_repos(const std::vector<std::filesystem::path>& all_repos,
                std::map<std::filesystem::path, RepoInfo>& repo_infos,
                std::set<std::filesystem::path>& skip_repos, std::mutex& mtx,
                std::atomic<bool>& scanning_flag, std::atomic<bool>& running, std::string& action,
                std::mutex& action_mtx, bool include_private, const std::string& remote,
                const std::filesystem::path& log_dir, bool check_only, bool hash_check,
                size_t concurrency, double cpu_percent_limit, size_t mem_limit, size_t down_limit,
                size_t up_limit, size_t disk_limit, bool silent, bool cli_mode, bool dry_run,
                bool force_pull, bool skip_timeout, bool skip_unavailable,
                bool skip_accessible_errors, const std::filesystem::path& post_pull_hook,
                const std::optional<std::string>& pull_ref, std::chrono::seconds updated_since,
                bool show_pull_author, std::chrono::seconds pull_timeout, bool retry_skipped,
                bool reset_skipped,
                const std::map<std::filesystem::path, RepoOptions>& repo_settings,
                bool mutant_mode);

void run_post_pull_hook(const std::filesystem::path& hook);

#endif // SCANNER_HPP
