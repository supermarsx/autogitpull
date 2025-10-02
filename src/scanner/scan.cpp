#include "scanner.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "debug_utils.hpp"
#include "ui_loop.hpp"
#include "git_utils.hpp"
#include "logger.hpp"
#include "resource_utils.hpp"
#include "thread_compat.hpp"

namespace fs = std::filesystem;

void scan_repos(const std::vector<fs::path>& all_repos, std::map<fs::path, RepoInfo>& repo_infos,
                std::set<fs::path>& skip_repos, std::mutex& mtx, std::atomic<bool>& scanning_flag,
                std::atomic<bool>& running, std::string& action, std::mutex& action_mtx,
                bool include_private, const std::string& remote, const fs::path& log_dir,
                bool check_only, bool hash_check, size_t concurrency, double cpu_percent_limit,
                size_t mem_limit, size_t down_limit, size_t up_limit, size_t disk_limit,
                bool silent, bool cli_mode, bool dry_run, bool force_pull, bool skip_timeout,
                bool skip_unavailable, bool skip_accessible_errors, const fs::path& post_pull_hook,
                const std::optional<std::string>& pull_ref, std::chrono::seconds updated_since,
                bool show_pull_author, std::chrono::seconds pull_timeout, bool retry_skipped,
                bool reset_skipped,
                const std::map<std::filesystem::path, RepoOptions>& repo_settings,
                bool mutant_mode) {
    git::GitInitGuard guard;
    static size_t last_mem = 0;
    size_t mem_before = procutil::get_memory_usage_mb();
    size_t virt_before = procutil::get_virtual_memory_kb();

    {
        std::lock_guard<std::mutex> lk(mtx);
        for (auto& [p, info] : repo_infos) {
            if (skip_repos.count(p)) {
                if (reset_skipped && info.status != RS_NOT_GIT) {
                    info.status = RS_PENDING;
                    info.message = "Pending...";
                    info.progress = 0;
                }
                if (!retry_skipped)
                    continue;
            }
            if (info.status != RS_NOT_GIT) {
                info.status = RS_PENDING;
                info.message = "Pending...";
                info.progress = 0;
            }
        }
        if (retry_skipped)
            skip_repos.clear();
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
                if (!retry_skipped && skip_repos.count(p))
                    continue;
                RepoOptions ro;
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    auto it_ro = repo_settings.find(p);
                    if (it_ro != repo_settings.end())
                        ro = it_ro->second;
                }
                fs::path repo_hook = ro.post_pull_hook.value_or(post_pull_hook);
                if (ro.exclude.value_or(false)) {
                    std::lock_guard<std::mutex> lk(mtx);
                    RepoInfo& info = repo_infos[p];
                    if (info.path.empty())
                        info.path = p;
                    info.status = RS_SKIPPED;
                    info.message = "Excluded";
                    continue;
                }
                bool co = ro.check_only.value_or(check_only);
                double cl = ro.cpu_limit.value_or(cpu_percent_limit);
                size_t dl = ro.download_limit.value_or(down_limit);
                size_t ul = ro.upload_limit.value_or(up_limit);
                size_t disk = ro.disk_limit.value_or(disk_limit);
                bool fp = ro.force_pull.value_or(force_pull);
                std::chrono::seconds pt = ro.pull_timeout.value_or(pull_timeout);
                std::optional<std::string> repo_target = ro.pull_ref;
                if (!repo_target && pull_ref)
                    repo_target = pull_ref;
                process_repo(p, repo_infos, skip_repos, mtx, running, action, action_mtx,
                             include_private, remote, log_dir, co, hash_check, dl, ul, disk, silent,
                             cli_mode, dry_run, fp, skip_timeout, skip_unavailable,
                             skip_accessible_errors, repo_hook, repo_target, updated_since,
                             show_pull_author, pt, mutant_mode);
                if (mem_limit > 0 && procutil::get_memory_usage_mb() > mem_limit) {
                    log_error("Memory limit exceeded");
                    running = false;
                    break;
                }
                if (cl > 0.0) {
                    double cpu = procutil::get_cpu_percent();
                    if (cpu > cl) {
                        double over = cpu / cl - 1.0;
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
