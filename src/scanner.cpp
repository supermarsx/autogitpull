#include "scanner.hpp"

#include <algorithm>
#include <fstream>
#include <thread>
#include <chrono>
#include <functional>
#include <iostream>
#include <ctime>

#include "git_utils.hpp"
#include "logger.hpp"
#include "time_utils.hpp"
#include "resource_utils.hpp"
#include "system_utils.hpp"
#include "debug_utils.hpp"
#include "thread_compat.hpp"
#include "ui_loop.hpp"

namespace fs = std::filesystem;

std::vector<fs::path> build_repo_list(const fs::path& root, bool recursive,
                                      const std::vector<fs::path>& ignore, size_t max_depth) {
    std::vector<fs::path> result;
    if (recursive) {
        std::error_code ec;
        fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied,
                                            ec);
        fs::recursive_directory_iterator end;
        for (; it != end; it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            if (max_depth > 0 && it.depth() >= static_cast<int>(max_depth)) {
                it.disable_recursion_pending();
                continue;
            }
            if (!it->is_directory(ec)) {
                if (ec)
                    ec.clear();
                continue;
            }
            fs::path p = it->path();
            if (std::find(ignore.begin(), ignore.end(), p) != ignore.end()) {
                it.disable_recursion_pending();
                continue;
            }
            result.push_back(p);
        }
    } else {
        std::error_code ec;
        fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
        fs::directory_iterator end;
        for (; it != end; it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            if (!it->is_directory(ec)) {
                if (ec)
                    ec.clear();
                continue;
            }
            fs::path p = it->path();
            if (std::find(ignore.begin(), ignore.end(), p) != ignore.end())
                continue;
            result.push_back(p);
        }
    }
    return result;
}

static bool validate_repo(const fs::path& p, RepoInfo& ri, std::set<fs::path>& skip_repos,
                          bool include_private) {
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
    ri.commit = git::get_local_hash(p);
    if (ri.commit.size() > 7)
        ri.commit = ri.commit.substr(0, 7);
    std::string origin = git::get_origin_url(p);
    if (!include_private) {
        if (!git::is_github_url(origin)) {
            ri.status = RS_SKIPPED;
            ri.message = "Non-GitHub repo (skipped)";
            if (logger_initialized())
                log_debug(p.string() + " skipped: non-GitHub repo");
            return false;
        }
        if (!git::remote_accessible(p)) {
            ri.status = RS_SKIPPED;
            ri.message = "Private or inaccessible repo";
            if (logger_initialized())
                log_debug(p.string() + " skipped: private or inaccessible");
            return false;
        }
    }
    ri.branch = git::get_current_branch(p);
    if (ri.branch.empty() || ri.branch == "HEAD") {
        ri.status = RS_HEAD_PROBLEM;
        ri.message = "Detached HEAD or branch error";
        skip_repos.insert(p);
        return false;
    }
    return true;
}

// Determine whether a pull is required and set RepoInfo accordingly
static bool determine_pull_action(const fs::path& p, RepoInfo& ri, bool check_only, bool hash_check,
                                  bool include_private, std::set<fs::path>& skip_repos,
                                  bool was_accessible, bool skip_accessible_errors) {
    if (hash_check) {
        std::string local = git::get_local_hash(p);
        bool auth_fail = false;
        std::string remote = git::get_remote_hash(p, ri.branch, include_private, &auth_fail);
        ri.auth_failed = auth_fail;
        if (local.empty() || remote.empty()) {
            ri.status = RS_ERROR;
            ri.message = "Error getting hashes or remote";
            if (!was_accessible || skip_accessible_errors)
                skip_repos.insert(p);
            else
                std::this_thread::sleep_for(std::chrono::seconds(1));
            return false;
        }
        if (local == remote) {
            ri.status = RS_UP_TO_DATE;
            ri.message = "Up to date";
            ri.commit = local.substr(0, 7);
            return false;
        }
    }

    if (check_only) {
        ri.status = RS_REMOTE_AHEAD;
        ri.message = hash_check ? "Remote ahead" : "Update possible";
        ri.commit = git::get_local_hash(p);
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

// Execute the git pull and update RepoInfo
static void execute_pull(const fs::path& p, RepoInfo& ri, std::map<fs::path, RepoInfo>& repo_infos,
                         std::set<fs::path>& skip_repos, std::mutex& mtx, std::string& action,
                         std::mutex& action_mtx, const fs::path& log_dir, bool include_private,
                         size_t down_limit, size_t up_limit, size_t disk_limit, bool force_pull,
                         bool was_accessible, bool skip_timeout, bool skip_accessible_errors,
                         bool cli_mode, bool silent, std::chrono::seconds pull_timeout) {
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
    int code = git::try_pull(p, pull_log, &progress_cb, include_private, &pull_auth_fail,
                             down_limit, up_limit, disk_limit, force_pull);
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
        ri.commit = git::get_local_hash(p);
        if (ri.commit.size() > 7)
            ri.commit = ri.commit.substr(0, 7);
        ri.pulled = true;
        if (logger_initialized())
            log_info(p.string() + " pulled successfully");
    } else if (code == 1) {
        ri.status = RS_PKGLOCK_FIXED;
        ri.message = "package-lock.json auto-reset & pulled";
        ri.commit = git::get_local_hash(p);
        if (ri.commit.size() > 7)
            ri.commit = ri.commit.substr(0, 7);
        ri.pulled = true;
        if (logger_initialized())
            log_info(p.string() + " package-lock reset and pulled");
    } else if (code == 3) {
        ri.status = RS_DIRTY;
        ri.message = "Local changes present";
    } else if (code == git::TRY_PULL_TIMEOUT) {
        ri.status = RS_ERROR;
        ri.message = "Pull timed out";
        if (skip_timeout && (!was_accessible || skip_accessible_errors))
            skip_repos.insert(p);
        else if (was_accessible)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        if (logger_initialized())
            log_error(p.string() + " pull timed out");
        if (cli_mode && !silent)
            std::cout << "Timed out " << p.filename().string() << std::endl;
    } else {
        ri.status = RS_ERROR;
        ri.message = "Pull failed (see log)";
        if (!was_accessible || skip_accessible_errors)
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
}

// Process a single repository
void process_repo(const fs::path& p, std::map<fs::path, RepoInfo>& repo_infos,
                  std::set<fs::path>& skip_repos, std::mutex& mtx, std::atomic<bool>& running,
                  std::string& action, std::mutex& action_mtx, bool include_private,
                  const fs::path& log_dir, bool check_only, bool hash_check, size_t down_limit,
                  size_t up_limit, size_t disk_limit, bool silent, bool cli_mode, bool force_pull,
                  bool skip_timeout, bool skip_accessible_errors,
                  std::chrono::seconds updated_since, bool show_pull_author,
                  std::chrono::seconds pull_timeout) {
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
                // Repo already being processed elsewhere
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
    {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = repo_infos.find(p);
        if (it != repo_infos.end()) {
            prev_pulled = it->second.pulled;
            ri.pulled = it->second.pulled;
            if (it->second.status != RS_ERROR && it->second.status != RS_SKIPPED)
                was_accessible = true;
        }
    }
    {
        std::lock_guard<std::mutex> lk(action_mtx);
        action = "Checking " + p.filename().string();
    }
    try {
        if (!validate_repo(p, ri, skip_repos, include_private)) {
            std::lock_guard<std::mutex> lk(mtx);
            repo_infos[p] = ri;
            return;
        }
        ri.commit_author = git::get_last_commit_author(p);
        ri.commit_date = git::get_last_commit_date(p);
        if (updated_since.count() > 0) {
            std::time_t ct = git::get_remote_commit_time(p, ri.branch, include_private, nullptr);
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

        bool do_pull = determine_pull_action(p, ri, check_only, hash_check, include_private,
                                             skip_repos, was_accessible, skip_accessible_errors);
        if (do_pull) {
            execute_pull(p, ri, repo_infos, skip_repos, mtx, action, action_mtx, log_dir,
                         include_private, down_limit, up_limit, disk_limit, force_pull,
                         was_accessible, skip_timeout, skip_accessible_errors, cli_mode, silent,
                         pull_timeout);
        }
    } catch (const fs::filesystem_error& e) {
        ri.status = RS_ERROR;
        ri.message = e.what();
        if (!was_accessible || skip_accessible_errors)
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
        std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&now));
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

// Background thread: scan and update info
void scan_repos(const std::vector<fs::path>& all_repos, std::map<fs::path, RepoInfo>& repo_infos,
                std::set<fs::path>& skip_repos, std::mutex& mtx, std::atomic<bool>& scanning_flag,
                std::atomic<bool>& running, std::string& action, std::mutex& action_mtx,
                bool include_private, const fs::path& log_dir, bool check_only, bool hash_check,
                size_t concurrency, double cpu_percent_limit, size_t mem_limit, size_t down_limit,
                size_t up_limit, size_t disk_limit, bool silent, bool cli_mode, bool force_pull,
                bool skip_timeout, bool skip_accessible_errors, std::chrono::seconds updated_since,
                bool show_pull_author, std::chrono::seconds pull_timeout,
                const std::map<std::filesystem::path, RepoOptions>& overrides) {
    git::GitInitGuard guard;
    static size_t last_mem = 0;
    size_t mem_before = procutil::get_memory_usage_mb();
    size_t virt_before = procutil::get_virtual_memory_kb();

    {
        std::lock_guard<std::mutex> lk(mtx);
        // Retain repo_infos so statuses persist between scans, but reset any
        // skipped repositories so they show up as Pending before the next scan
        // begins. Otherwise they disappear from the status list until their
        // status is updated.
        for (auto& [p, info] : repo_infos) {
            if (info.status == RS_SKIPPED) {
                info.status = RS_PENDING;
                info.message = "Pending...";
            }
        }
        std::set<fs::path> empty_set;
        skip_repos.swap(empty_set);
    }

    if (concurrency == 0)
        concurrency = 1;
    concurrency = std::min(concurrency, all_repos.size());
    if (logger_initialized())
        log_debug("Scanning repositories");

    std::atomic<size_t> next_index{0};
    auto worker = [&]() {
        try {
            while (running) {
                size_t idx = next_index.fetch_add(1);
                if (idx >= all_repos.size())
                    break;
                const auto& p = all_repos[idx];
                RepoOptions ro;
                auto it_ro = overrides.find(p);
                if (it_ro != overrides.end())
                    ro = it_ro->second;
                size_t dl = ro.download_limit.value_or(down_limit);
                size_t ul = ro.upload_limit.value_or(up_limit);
                size_t disk = ro.disk_limit.value_or(disk_limit);
                bool fp = ro.force_pull.value_or(force_pull);
                std::chrono::seconds pt = ro.pull_timeout.value_or(pull_timeout);
                process_repo(p, repo_infos, skip_repos, mtx, running, action, action_mtx,
                             include_private, log_dir, check_only, hash_check, dl, ul, disk, silent,
                             cli_mode, fp, skip_timeout, skip_accessible_errors, updated_since,
                             show_pull_author, pt);
                if (mem_limit > 0 && procutil::get_memory_usage_mb() > mem_limit) {
                    log_error("Memory limit exceeded");
                    running = false;
                    break;
                }
                if (cpu_percent_limit > 0.0) {
                    double cpu = procutil::get_cpu_percent();
                    if (cpu > cpu_percent_limit) {
                        double over = cpu / cpu_percent_limit - 1.0;
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(static_cast<int>(over * 100)));
                    }
                }
            }
        } catch (const std::exception& e) {
            log_error(std::string("Worker thread exception: ") + e.what());
            running = false;
        } catch (...) {
            log_error("Worker thread unknown exception");
            running = false;
        }
    };

    std::vector<th_compat::jthread> threads;
    threads.reserve(concurrency);
    const size_t max_threads = concurrency;
    for (size_t i = 0; i < max_threads; ++i) {
        if (threads.size() >= max_threads)
            break;
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }
    threads.clear();
    threads.shrink_to_fit();
    if (debugMemory || dumpState) {
        size_t mem_after = procutil::get_memory_usage_mb();
        size_t virt_after = procutil::get_virtual_memory_kb();
        long long mem_delta =
            static_cast<long long>(mem_after) - static_cast<long long>(mem_before);
        long long vmem_delta =
            static_cast<long long>(virt_after) - static_cast<long long>(virt_before);
        log_debug("Memory before=" + std::to_string(mem_before) + "MB after=" +
                  std::to_string(mem_after) + "MB delta=" + std::to_string(mem_delta) +
                  "MB vmem_before=" + std::to_string(virt_before / 1024) +
                  "MB vmem_after=" + std::to_string(virt_after / 1024) +
                  "MB vmem_delta=" + std::to_string(vmem_delta / 1024) + "MB");
        debug_utils::log_memory_delta_mb(mem_after, last_mem);
        debug_utils::log_container_size("repo_infos", repo_infos);
        debug_utils::log_container_size("skip_repos", skip_repos);
        if (dumpState && repo_infos.size() > dumpThreshold)
            debug_utils::dump_repo_infos(repo_infos, dumpThreshold);
        if (dumpState && skip_repos.size() > dumpThreshold)
            debug_utils::dump_container("skip_repos", skip_repos, dumpThreshold);
    }
    scanning_flag = false;
    {
        std::lock_guard<std::mutex> lk(action_mtx);
        action = "Idle";
    }
    if (logger_initialized())
        log_debug("Scan complete");
}
