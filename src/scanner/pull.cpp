#include <filesystem>
#include <functional>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <iostream>
#include <thread>

#include "git_utils.hpp"
#include "logger.hpp"
#include "scanner.hpp"
#include "time_utils.hpp"

namespace fs = std::filesystem;

namespace scanner_detail {

void execute_pull(const fs::path& p, RepoInfo& ri, std::map<fs::path, RepoInfo>& repo_infos,
                  std::set<fs::path>& skip_repos, std::mutex& mtx, std::string& action,
                  std::mutex& action_mtx, const fs::path& log_dir, bool include_private,
                  const std::string& remote, size_t down_limit, size_t up_limit, size_t disk_limit,
                  bool force_pull, bool was_accessible, bool /*skip_timeout*/, bool skip_unavailable,
                  bool skip_accessible_errors, bool cli_mode, bool silent,
                  const fs::path& post_pull_hook, const std::optional<std::string>& pull_ref,
                  std::chrono::seconds pull_timeout) {
    {
        std::lock_guard<std::mutex> lk(action_mtx);
        action = "Pulling " + p.filename().string();
    }
    {
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
    }

    std::string pull_log;
    std::function<void(int)> progress_cb = [&](int pct) {
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p].progress = pct;
    };
    bool pull_auth_fail = false;
    if (pull_timeout.count() > 0)
        git::set_libgit_timeout(static_cast<unsigned int>(pull_timeout.count()));
    const std::string* target_ref_ptr = nullptr;
    if (pull_ref && !pull_ref->empty())
        target_ref_ptr = &(*pull_ref);
    int code = git::try_pull(p, remote, pull_log, &progress_cb, include_private, &pull_auth_fail,
                             down_limit, up_limit, disk_limit, force_pull, target_ref_ptr);
    ri.auth_failed = pull_auth_fail;
    ri.last_pull_log = pull_log;

    fs::path log_file_path;
    if (!log_dir.empty()) {
        std::string ts = timestamp();
        for (char& ch : ts) {
            if (ch == ' ' || ch == ':')
                ch = '_';
            else if (ch == '/')
                ch = '-';
        }
        log_file_path = log_dir / (p.filename().string() + "_" + ts + ".log");
        std::ofstream ofs(log_file_path);
        ofs << pull_log;
    }

    if (code == 0) {
        ri.status = RS_PULL_OK;
        ri.message = "Pulled successfully";
        ri.commit = git::get_local_hash(p).value_or("");
        if (ri.commit.size() > 7)
            ri.commit = ri.commit.substr(0, 7);
        ri.pulled = true;
        if (logger_initialized())
            log_info(p.string() + " pulled successfully");
    } else if (code == 1) {
        ri.status = RS_PKGLOCK_FIXED;
        ri.message = "package-lock.json auto-reset & pulled";
        ri.commit = git::get_local_hash(p).value_or("");
        if (ri.commit.size() > 7)
            ri.commit = ri.commit.substr(0, 7);
        ri.pulled = true;
        if (logger_initialized())
            log_info(p.string() + " package-lock reset and pulled");
    } else if (code == 3) {
        ri.status = RS_DIRTY;
        ri.message = "Local changes present";
    } else if (code == git::TRY_PULL_TIMEOUT) {
        ri.status = RS_TIMEOUT;
        ri.message = "Pull timed out";
        if (was_accessible)
            std::this_thread::sleep_for(std::chrono::seconds(5));
        if (logger_initialized())
            log_error(p.string() + " pull timed out");
        if (cli_mode && !silent)
            std::cout << "Timed out " << p.filename().string() << std::endl;
    } else if (code == git::TRY_PULL_RATE_LIMIT) {
        ri.status = RS_RATE_LIMIT;
        ri.message = "Rate limited";
        if (was_accessible)
            std::this_thread::sleep_for(std::chrono::seconds(5));
        if (logger_initialized())
            log_error(p.string() + " rate limited");
        if (cli_mode && !silent)
            std::cout << "Rate limited " << p.filename().string() << std::endl;
    } else {
        ri.status = RS_ERROR;
        ri.message = "Pull failed (see log)";
        if ((skip_unavailable && !was_accessible) || skip_accessible_errors)
            skip_repos.insert(p);
        else
            std::this_thread::sleep_for(std::chrono::seconds(1));
        if (logger_initialized())
            log_error(p.string() + " pull failed");
    }

    if (!log_file_path.empty())
        ri.message += " - " + log_file_path.string();
    ri.commit_author = git::get_last_commit_author(p);
    ri.commit_date = git::get_last_commit_date(p);
    ri.commit_time = git::get_last_commit_time(p);
    if (ri.pulled)
        run_post_pull_hook(post_pull_hook);
}

} // namespace scanner_detail
