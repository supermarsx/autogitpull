#include "scanner.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>

#include "debug_utils.hpp"
#include "git_utils.hpp"
#include "logger.hpp"
#include "mutant_mode.hpp"
#include "resource_utils.hpp"
#include "system_utils.hpp"
#include "thread_compat.hpp"
#include "time_utils.hpp"

namespace fs = std::filesystem;

static bool validate_repo(const fs::path& p, RepoInfo& ri, std::set<fs::path>& skip_repos,
                          bool include_private, bool prev_pulled, const std::string& remote) {
    if (!fs::exists(p)) {
        ri.status = RS_ERROR;
        ri.message = "Missing";
        if (logger_initialized())
            log_error(p.string() + " missing");
        return false;
    }
    if (skip_repos.count(p)) {
        ri.status = RS_SKIPPED;
        ri.message = "Skipped after fatal error";
        if (logger_initialized())
            log_warning(p.string() + " skipped after fatal error");
        return false;
    }
    ri.status = RS_CHECKING;
    ri.message = "";
    if (!fs::is_directory(p) || !git::is_git_repo(p)) {
        ri.status = RS_NOT_GIT;
        ri.message = "Not a git repo";
        if (logger_initialized())
            log_debug(p.string() + " tagged: not a git repo");
        return false;
    }
    ri.commit = git::get_local_hash(p).value_or("");
    if (ri.commit.size() > 7)
        ri.commit = ri.commit.substr(0, 7);
    std::string remote_url = git::get_remote_url(p, remote).value_or("");
    if (!include_private) {
        if (!git::is_github_url(remote_url)) {
            ri.status = RS_SKIPPED;
            ri.message = "Non-GitHub repo (skipped)";
            if (logger_initialized())
                log_debug(p.string() + " skipped: non-GitHub repo");
            return false;
        }
        if (!git::remote_accessible(p, remote)) {
            if (prev_pulled) {
                ri.status = RS_TEMPFAIL;
                ri.message = "Temporarily inaccessible";
                if (logger_initialized())
                    log_warning(p.string() + " temporarily inaccessible");
            } else {
                ri.status = RS_SKIPPED;
                ri.message = "Private or inaccessible repo";
                if (logger_initialized())
                    log_debug(p.string() + " skipped: private or inaccessible");
            }
            return false;
        }
    }
    ri.branch = git::get_current_branch(p).value_or("");
    if (ri.branch.empty() || ri.branch == "HEAD") {
        ri.status = RS_HEAD_PROBLEM;
        ri.message = "Detached HEAD or branch error";
        skip_repos.insert(p);
        return false;
    }
    return true;
}

static bool determine_pull_action(const fs::path& p, RepoInfo& ri, bool check_only, bool hash_check,
                                  bool include_private, std::set<fs::path>& skip_repos,
                                  bool was_accessible, bool skip_unavailable,
                                  bool skip_accessible_errors, const std::string& remote) {
    if (hash_check) {
        std::string local = git::get_local_hash(p).value_or("");
        bool auth_fail = false;
        std::string remote_hash =
            git::get_remote_hash(p, remote, ri.branch, include_private, &auth_fail).value_or("");
        ri.auth_failed = auth_fail;
        if (local.empty() || remote_hash.empty()) {
            ri.status = RS_ERROR;
            ri.message = "Error getting hashes or remote";
            if ((skip_unavailable && !was_accessible) || skip_accessible_errors)
                skip_repos.insert(p);
            else
                std::this_thread::sleep_for(std::chrono::seconds(1));
            return false;
        }
        if (local == remote_hash) {
            ri.status = RS_UP_TO_DATE;
            ri.message = "Up to date";
            ri.commit = local.substr(0, 7);
            return false;
        }
    }

    if (check_only) {
        ri.status = RS_REMOTE_AHEAD;
        ri.message = hash_check ? "Remote ahead" : "Update possible";
        ri.commit = git::get_local_hash(p).value_or("");
        if (ri.commit.size() > 7)
            ri.commit = ri.commit.substr(0, 7);
        if (logger_initialized())
            log_debug(p.string() + " remote ahead");
        return false;
    }

    ri.status = RS_PULLING;
    ri.message = "Remote ahead, pulling...";
    ri.progress = 0;
    return true;
}

namespace scanner_detail { void execute_pull(
    const fs::path& p, RepoInfo& ri, std::map<fs::path, RepoInfo>& repo_infos,
    std::set<fs::path>& skip_repos, std::mutex& mtx, std::string& action,
    std::mutex& action_mtx, const fs::path& log_dir, bool include_private,
    const std::string& remote, size_t down_limit, size_t up_limit, size_t disk_limit,
    bool force_pull, bool was_accessible, bool /*skip_timeout*/, bool skip_unavailable,
    bool skip_accessible_errors, bool cli_mode, bool silent, const fs::path& post_pull_hook,
    const std::optional<std::string>& pull_ref, std::chrono::seconds pull_timeout);
} // namespace scanner_detail

void process_repo(const fs::path& p, std::map<fs::path, RepoInfo>& repo_infos,
                  std::set<fs::path>& skip_repos, std::mutex& mtx, std::atomic<bool>& running,
                  std::string& action, std::mutex& action_mtx, bool include_private,
                  const std::string& remote, const fs::path& log_dir, bool check_only,
                  bool hash_check, size_t down_limit, size_t up_limit, size_t disk_limit,
                  bool silent, bool cli_mode, bool dry_run, bool force_pull, bool skip_timeout,
                  bool skip_unavailable, bool skip_accessible_errors,
                  const fs::path& post_pull_hook, const std::optional<std::string>& pull_ref,
                  std::chrono::seconds updated_since, bool show_pull_author,
                  std::chrono::seconds pull_timeout, bool mutant_mode) {
    if (!running)
        return;
    if (logger_initialized())
        log_debug("Checking repo " + p.string());
    {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = repo_infos.find(p);
        if (it != repo_infos.end()) {
            if (it->second.status == RS_NOT_GIT)
                return;
            if (it->second.status == RS_PULLING || it->second.status == RS_CHECKING) {
                if (!silent)
                    std::cerr << "Skipping " << p << " - busy\n";
                if (logger_initialized())
                    log_debug("Skipping " + p.string() + " - busy");
                return;
            }
        }
    }
    RepoInfo ri;
    ri.path = p;
    ri.auth_failed = false;
    bool prev_pulled = false;
    bool was_accessible = false;
    RepoStatus prev_status = RS_PENDING;
    {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = repo_infos.find(p);
        if (it != repo_infos.end()) {
            prev_pulled = it->second.pulled;
            ri.pulled = it->second.pulled;
            prev_status = it->second.status;
            if (it->second.status != RS_ERROR && it->second.status != RS_SKIPPED)
                was_accessible = true;
        }
    }
    std::chrono::seconds effective_timeout = pull_timeout;
    if (prev_status == RS_RATE_LIMIT) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    } else if (prev_status == RS_TIMEOUT) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (effective_timeout.count() > 0)
            effective_timeout += std::chrono::seconds(5);
        else
            effective_timeout = std::chrono::seconds(5);
    }
    {
        std::lock_guard<std::mutex> lk(action_mtx);
        action = "Checking " + p.filename().string();
    }
    try {
        if (!validate_repo(p, ri, skip_repos, include_private, prev_pulled, remote)) {
            std::lock_guard<std::mutex> lk(mtx);
            repo_infos[p] = ri;
            return;
        }
        ri.commit_author = git::get_last_commit_author(p);
        ri.commit_date = git::get_last_commit_date(p);
        ri.commit_time = git::get_last_commit_time(p);
        if (updated_since.count() > 0) {
            if (mutant_mode) {
                if (!mutant_should_pull(p, ri, remote, include_private, updated_since)) {
                    std::lock_guard<std::mutex> lk(mtx);
                    repo_infos[p] = ri;
                    return;
                }
            } else {
                std::time_t ct =
                    git::get_remote_commit_time(p, remote, ri.branch, include_private, nullptr);
                if (ct == 0)
                    ct = git::get_last_commit_time(p);
                std::time_t now = std::time(nullptr);
                if (ct == 0 || now - ct > updated_since.count()) {
                    ri.status = RS_SKIPPED;
                    ri.message = "Older than limit";
                    std::lock_guard<std::mutex> lk(mtx);
                    repo_infos[p] = ri;
                    return;
                }
            }
        }

        bool do_pull =
            determine_pull_action(p, ri, check_only, hash_check, include_private, skip_repos,
                                  was_accessible, skip_unavailable, skip_accessible_errors, remote);
        if (do_pull) {
            if (dry_run) {
                ri.status = RS_REMOTE_AHEAD;
                ri.message = "Dry run";
                ri.commit = git::get_local_hash(p).value_or("");
                if (ri.commit.size() > 7)
                    ri.commit = ri.commit.substr(0, 7);
            } else {
                auto start_time = std::chrono::steady_clock::now();
                scanner_detail::execute_pull(p, ri, repo_infos, skip_repos, mtx, action, action_mtx, log_dir,
                             include_private, remote, down_limit, up_limit, disk_limit, force_pull,
                             was_accessible, skip_timeout, skip_unavailable, skip_accessible_errors,
                             cli_mode, silent, post_pull_hook, pull_ref, effective_timeout);
                auto end_time = std::chrono::steady_clock::now();
                if (mutant_mode)
                    mutant_record_result(
                        p, ri.status,
                        std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time));
            }
        }
    } catch (const fs::filesystem_error& e) {
        ri.status = RS_ERROR;
        ri.message = e.what();
        if ((skip_unavailable && !was_accessible) || skip_accessible_errors)
            skip_repos.insert(p);
        else
            std::this_thread::sleep_for(std::chrono::seconds(1));
        if (logger_initialized())
            log_error(p.string() + " error: " + ri.message);
    }
    {
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
    }
    if (cli_mode && !silent && ri.pulled && !prev_pulled) {
        std::time_t now = std::time(nullptr);
        char buf[32];
#ifdef _WIN32
        std::tm tm_buf{};
        localtime_s(&tm_buf, &now);
        std::strftime(buf, sizeof(buf), "%F %T", &tm_buf);
#else
        std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&now));
#endif
        std::cout << "Updated " << p.filename().string();
        if (!ri.commit_date.empty())
            std::cout << " at " << ri.commit_date;
        else
            std::cout << " at " << buf;
        if (show_pull_author && !ri.commit_author.empty())
            std::cout << " by " << ri.commit_author;
        if (!ri.commit.empty())
            std::cout << ", commit " << ri.commit;
        std::cout << std::endl;
    }
    if (logger_initialized())
        log_debug(p.string() + " -> " + ri.message);
}
