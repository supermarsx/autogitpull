#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <set>
#include <map>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <sstream>
#include <cctype>
#include <csignal>
#include <memory>
#include <cstring>
#include "thread_compat.hpp"
#include "arg_parser.hpp"
#include "git_utils.hpp"
#include "tui.hpp"
#include "repo.hpp"
#include "logger.hpp"
#include "time_utils.hpp"
#include "resource_utils.hpp"
#include "system_utils.hpp"
#include "version.hpp"
#include "config_utils.hpp"
#include "debug_utils.hpp"
#include "parse_utils.hpp"
#include "options.hpp"
#include "lock_utils.hpp"
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include "linux_daemon.hpp"
#else
#include "windows_service.hpp"
#endif

namespace fs = std::filesystem;

struct AltScreenGuard {
    AltScreenGuard() {
        enable_win_ansi();
        std::cout << "\033[?1049h";
    }
    ~AltScreenGuard() { std::cout << "\033[?1049l" << std::flush; }
};

std::atomic<bool>* g_running_ptr = nullptr;
bool debugMemory = false;
bool dumpState = false;
size_t dumpThreshold = 0;

struct OptionInfo {
    const char* long_flag;
    const char* short_flag;
    const char* arg;
    const char* desc;
    const char* category;
};

void handle_signal(int) {
    if (g_running_ptr)
        g_running_ptr->store(false);
}

std::string status_label(RepoStatus status) {
    switch (status) {
    case RS_PENDING:
        return "Pending";
    case RS_CHECKING:
        return "Checking";
    case RS_UP_TO_DATE:
        return "UpToDate";
    case RS_PULLING:
        return "Pulling";
    case RS_PULL_OK:
        return "Pulled";
    case RS_PKGLOCK_FIXED:
        return "PkgLockOk";
    case RS_ERROR:
        return "Error";
    case RS_SKIPPED:
        return "Skipped";
    case RS_HEAD_PROBLEM:
        return "HEAD/BR";
    case RS_DIRTY:
        return "Dirty";
    case RS_REMOTE_AHEAD:
        return "RemoteUp";
    }
    return "";
}

/**
 * @brief Collect repository paths under @a root.
 *
 * @param root      Directory to scan.
 * @param recursive Whether to scan recursively.
 * @param ignore    List of directories to skip.
 * @return Vector of discovered directories.
 */
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
            fs::path p = it->path();
            if (std::find(ignore.begin(), ignore.end(), p) != ignore.end())
                continue;
            result.push_back(p);
        }
    }
    return result;
}

/** Print the command line help text. */
void print_help(const char* prog) {
    static const std::vector<OptionInfo> opts = {
        {"--include-private", "-p", "", "Include private repositories", "General"},
        {"--show-skipped", "-k", "", "Show skipped repositories", "General"},
        {"--show-version", "-v", "", "Display program version in TUI", "General"},
        {"--version", "-V", "", "Print program version and exit", "General"},
        {"--interval", "-i", "<sec>", "Delay between scans", "General"},
        {"--refresh-rate", "-r", "<ms>", "TUI refresh rate", "General"},
        {"--recursive", "", "", "Scan subdirectories recursively", "General"},
        {"--max-depth", "-D", "<n>", "Limit recursive scan depth", "General"},
        {"--ignore", "", "<dir>", "Directory to ignore (repeatable)", "General"},
        {"--config-yaml", "-y", "<file>", "Load options from YAML file", "General"},
        {"--config-json", "-j", "<file>", "Load options from JSON file", "General"},
        {"--cli", "-c", "", "Use console output", "General"},
        {"--single-run", "", "", "Run a single scan cycle and exit", "General"},
        {"--silent", "-s", "", "Disable console output", "General"},
        {"--check-only", "", "", "Only check for updates", "Actions"},
        {"--no-hash-check", "", "", "Always pull without hash check", "Actions"},
        {"--force-pull", "", "", "Discard local changes when pulling", "Actions"},
        {"--discard-dirty", "", "", "Alias for --force-pull", "Actions"},
        {"--remove-lock", "-R", "", "Remove directory lock file and exit", "Actions"},
        {"--log-dir", "-d", "<path>", "Directory for pull logs", "Logging"},
        {"--log-file", "-l", "<path>", "File for general logs", "Logging"},
        {"--log-level", "", "<level>", "Set log verbosity", "Logging"},
        {"--verbose", "", "", "Shorthand for --log-level DEBUG", "Logging"},
        {"--debug-memory", "", "", "Log memory usage each scan", "Logging"},
        {"--dump-state", "", "", "Dump container state when large", "Logging"},
        {"--dump-large", "", "<n>", "Dump threshold for --dump-state", "Logging"},
        {"--concurrency", "", "<n>", "Number of worker threads", "Concurrency"},
        {"--threads", "", "<n>", "Alias for --concurrency", "Concurrency"},
        {"--single-thread", "", "", "Run using a single worker thread", "Concurrency"},
        {"--max-threads", "", "<n>", "Cap the scanning worker threads", "Concurrency"},
        {"--cpu-poll", "", "<s>", "CPU usage polling interval", "Tracking"},
        {"--mem-poll", "", "<s>", "Memory usage polling interval", "Tracking"},
        {"--thread-poll", "", "<s>", "Thread count polling interval", "Tracking"},
        {"--no-cpu-tracker", "", "", "Disable CPU usage tracker", "Tracking"},
        {"--no-mem-tracker", "", "", "Disable memory usage tracker", "Tracking"},
        {"--no-thread-tracker", "", "", "Disable thread tracker", "Tracking"},
        {"--net-tracker", "", "", "Track network usage", "Tracking"},
        {"--cpu-percent", "", "<n>", "Approximate CPU usage limit", "Resource limits"},
        {"--cpu-cores", "", "<mask>", "Set CPU affinity mask", "Resource limits"},
        {"--mem-limit", "", "<MB>", "Abort if memory exceeds this amount", "Resource limits"},
        {"--download-limit", "", "<KB/s>", "Limit total download rate", "Resource limits"},
        {"--upload-limit", "", "<KB/s>", "Limit total upload rate", "Resource limits"},
        {"--disk-limit", "", "<KB/s>", "Limit disk throughput", "Resource limits"},
        {"--help", "-h", "", "Show this message", "General"}};

    std::map<std::string, std::vector<const OptionInfo*>> groups;
    size_t width = 0;
    for (const auto& o : opts) {
        groups[o.category].push_back(&o);
        std::string flag = "  ";
        if (std::strlen(o.short_flag))
            flag += std::string(o.short_flag) + ", ";
        else
            flag += "    ";
        flag += o.long_flag;
        if (std::strlen(o.arg))
            flag += " " + std::string(o.arg);
        width = std::max(width, flag.size());
    }

    std::cout << "autogitpull - Automatic Git Puller & Monitor\n";
    std::cout << "Scans a directory of Git repositories and pulls updates.\n";
    std::cout << "Configuration can be read from YAML or JSON files.\n\n";
    std::cout << "Usage: " << prog << " <root-folder> [options]\n\n";
    const std::vector<std::string> order{"General",         "Logging",  "Concurrency",
                                         "Resource limits", "Tracking", "Actions"};
    for (const auto& cat : order) {
        if (!groups.count(cat))
            continue;
        std::cout << cat << ":\n";
        for (const auto* o : groups[cat]) {
            std::string flag = "  ";
            if (std::strlen(o->short_flag))
                flag += std::string(o->short_flag) + ", ";
            else
                flag += "    ";
            flag += o->long_flag;
            if (std::strlen(o->arg))
                flag += " " + std::string(o->arg);
            std::cout << std::left << std::setw(static_cast<int>(width) + 2) << flag << o->desc
                      << "\n";
        }
        std::cout << "\n";
    }
}

void draw_cli(const std::vector<fs::path>& all_repos,
              const std::map<fs::path, RepoInfo>& repo_infos, int seconds_left, bool scanning,
              const std::string& action, bool show_skipped) {
    std::cout << "Status: ";
    if (scanning)
        std::cout << action;
    else
        std::cout << "Idle";
    std::cout << " - Next scan in " << seconds_left << "s\n";
    for (const auto& p : all_repos) {
        RepoInfo ri;
        auto it = repo_infos.find(p);
        if (it != repo_infos.end())
            ri = it->second;
        else
            ri = RepoInfo{p, RS_PENDING, "Pending...", "", "", "", 0, false};
        if (ri.status == RS_SKIPPED && !show_skipped)
            continue;
        std::cout << " [" << status_label(ri.status) << "] " << p.filename().string();
        if (!ri.branch.empty()) {
            std::cout << " (" << ri.branch;
            if (!ri.commit.empty())
                std::cout << "@" << ri.commit;
            std::cout << ")";
        }
        if (!ri.message.empty())
            std::cout << " - " << ri.message;
        if (ri.status == RS_PULLING)
            std::cout << " (" << ri.progress << "%)";
        if (ri.auth_failed)
            std::cout << " [AUTH]";
        std::cout << "\n";
    }
    std::cout << std::flush;
}

// Apply process and tracking related settings
static void setup_environment(const Options& opts) {
    if (opts.cpu_core_mask != 0)
        procutil::set_cpu_affinity(opts.cpu_core_mask);
    procutil::set_cpu_poll_interval(opts.cpu_poll_sec);
    procutil::set_memory_poll_interval(opts.mem_poll_sec);
    procutil::set_thread_poll_interval(opts.thread_poll_sec);
    if (opts.net_tracker)
        procutil::init_network_usage();
}

// Initialize logging if requested
static void setup_logging(const Options& opts) {
    if (!opts.log_file.empty()) {
        init_logger(opts.log_file, opts.log_level);
        if (logger_initialized())
            log_info("Program started");
    }
}

// Build repository list and populate info table
static void prepare_repos(const Options& opts, std::vector<fs::path>& all_repos,
                          std::map<fs::path, RepoInfo>& repo_infos) {
    all_repos = build_repo_list(opts.root, opts.recursive_scan, opts.ignore_dirs, opts.max_depth);
    for (const auto& p : all_repos)
        repo_infos[p] = RepoInfo{p, RS_PENDING, "Pending...", "", "", "", 0, false};
}

// Render either the TUI or CLI output
static void update_ui(const Options& opts, const std::vector<fs::path>& all_repos,
                      const std::map<fs::path, RepoInfo>& repo_infos, int sec_left, bool scanning,
                      const std::string& act, std::chrono::milliseconds& cli_countdown_ms) {
    if (!opts.silent && !opts.cli) {
        draw_tui(all_repos, repo_infos, opts.interval, sec_left, scanning, act, opts.show_skipped,
                 opts.show_version, opts.cpu_tracker, opts.mem_tracker, opts.thread_tracker,
                 opts.net_tracker, opts.cpu_core_mask != 0);
    } else if (!opts.silent && opts.cli && cli_countdown_ms <= std::chrono::milliseconds(0)) {
        draw_cli(all_repos, repo_infos, sec_left, scanning, act, opts.show_skipped);
        cli_countdown_ms = std::chrono::milliseconds(1000);
    }
}

// Validate repository and gather basic info
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
        ri.status = RS_SKIPPED;
        ri.message = "Not a git repo (skipped)";
        skip_repos.insert(p);
        if (logger_initialized())
            log_debug(p.string() + " skipped: not a git repo");
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
                                  bool include_private, std::set<fs::path>& skip_repos) {
    if (hash_check) {
        std::string local = git::get_local_hash(p);
        bool auth_fail = false;
        std::string remote = git::get_remote_hash(p, ri.branch, include_private, &auth_fail);
        ri.auth_failed = auth_fail;
        if (local.empty() || remote.empty()) {
            ri.status = RS_ERROR;
            ri.message = "Error getting hashes or remote";
            skip_repos.insert(p);
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
                         size_t down_limit, size_t up_limit, size_t disk_limit, bool force_pull) {
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
        if (logger_initialized())
            log_info(p.string() + " pulled successfully");
    } else if (code == 1) {
        ri.status = RS_PKGLOCK_FIXED;
        ri.message = "package-lock.json auto-reset & pulled";
        ri.commit = git::get_local_hash(p);
        if (ri.commit.size() > 7)
            ri.commit = ri.commit.substr(0, 7);
        if (logger_initialized())
            log_info(p.string() + " package-lock reset and pulled");
    } else if (code == 3) {
        ri.status = RS_DIRTY;
        ri.message = "Local changes present";
    } else {
        ri.status = RS_ERROR;
        ri.message = "Pull failed (see log)";
        skip_repos.insert(p);
        if (logger_initialized())
            log_error(p.string() + " pull failed");
    }

    if (!log_file_path.empty())
        ri.message += " - " + log_file_path.string();
}

// Process a single repository
void process_repo(const fs::path& p, std::map<fs::path, RepoInfo>& repo_infos,
                  std::set<fs::path>& skip_repos, std::mutex& mtx, std::atomic<bool>& running,
                  std::string& action, std::mutex& action_mtx, bool include_private,
                  const fs::path& log_dir, bool check_only, bool hash_check, size_t down_limit,
                  size_t up_limit, size_t disk_limit, bool silent, bool force_pull) {
    if (!running)
        return;
    if (logger_initialized())
        log_debug("Checking repo " + p.string());
    {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = repo_infos.find(p);
        if (it != repo_infos.end() &&
            (it->second.status == RS_PULLING || it->second.status == RS_CHECKING)) {
            // Repo already being processed elsewhere
            if (!silent)
                std::cerr << "Skipping " << p << " - busy\n";
            if (logger_initialized())
                log_debug("Skipping " + p.string() + " - busy");
            return;
        }
    }
    RepoInfo ri;
    ri.path = p;
    ri.auth_failed = false;
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

        bool do_pull =
            determine_pull_action(p, ri, check_only, hash_check, include_private, skip_repos);
        if (do_pull) {
            execute_pull(p, ri, repo_infos, skip_repos, mtx, action, action_mtx, log_dir,
                         include_private, down_limit, up_limit, disk_limit, force_pull);
        }
    } catch (const fs::filesystem_error& e) {
        ri.status = RS_ERROR;
        ri.message = e.what();
        skip_repos.insert(p);
        if (logger_initialized())
            log_error(p.string() + " error: " + ri.message);
    }
    {
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
    }
    if (logger_initialized())
        log_debug(p.string() + " -> " + ri.message);
}

// Background thread: scan and update info
void scan_repos(const std::vector<fs::path>& all_repos, std::map<fs::path, RepoInfo>& repo_infos,
                std::set<fs::path>& skip_repos, std::mutex& mtx, std::atomic<bool>& scanning_flag,
                std::atomic<bool>& running, std::string& action, std::mutex& action_mtx,
                bool include_private, const fs::path& log_dir, bool check_only, bool hash_check,
                size_t concurrency, int cpu_percent_limit, size_t mem_limit, size_t down_limit,
                size_t up_limit, size_t disk_limit, bool silent, bool force_pull) {
    git::GitInitGuard guard;
    static size_t last_mem = 0;
    size_t mem_before = procutil::get_memory_usage_mb();
    size_t virt_before = procutil::get_virtual_memory_kb();

    {
        std::lock_guard<std::mutex> lk(mtx);
        // Retain repo_infos so statuses persist between scans
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
        while (running) {
            size_t idx = next_index.fetch_add(1);
            if (idx >= all_repos.size())
                break;
            const auto& p = all_repos[idx];
            process_repo(p, repo_infos, skip_repos, mtx, running, action, action_mtx,
                         include_private, log_dir, check_only, hash_check, down_limit, up_limit,
                         disk_limit, silent, force_pull);
            if (mem_limit > 0 && procutil::get_memory_usage_mb() > mem_limit) {
                log_error("Memory limit exceeded");
                running = false;
                break;
            }
            if (cpu_percent_limit > 0) {
                double cpu = procutil::get_cpu_percent();
                if (cpu > cpu_percent_limit) {
                    double over = cpu / cpu_percent_limit - 1.0;
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(static_cast<int>(over * 100)));
                }
            }
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
        log_debug("Memory before=" + std::to_string(mem_before) + "MB after=" +
                  std::to_string(mem_after) + "MB delta=" + std::to_string(mem_after - mem_before) +
                  "MB vmem_before=" + std::to_string(virt_before / 1024) +
                  "MB vmem_after=" + std::to_string(virt_after / 1024) +
                  "MB vmem_delta=" + std::to_string((virt_after - virt_before) / 1024) + "MB");
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

int run_event_loop(const Options& opts) {
    debugMemory = opts.debug_memory;
    dumpState = opts.dump_state;
    dumpThreshold = opts.dump_threshold;
#ifndef _WIN32
    if (opts.root.empty() && !opts.attach_name.empty()) {
        int fd = procutil::connect_status_socket(opts.attach_name);
        if (fd < 0) {
            std::cerr << "Failed to connect to daemon" << std::endl;
            return 1;
        }
        char buf[256];
        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = 0;
            std::cout << buf << std::flush;
        }
        close(fd);
        return 0;
    }
#endif
    if (opts.root.empty())
        return 0;
    if (!fs::exists(opts.root) || !fs::is_directory(opts.root))
        throw std::runtime_error("Root path does not exist or is not a directory.");
    fs::path lock_path = opts.root / ".autogitpull.lock";
    procutil::LockFileGuard lock(lock_path);
    if (!lock.locked) {
        unsigned long pid = 0;
        if (procutil::read_lock_pid(lock_path, pid) && procutil::process_running(pid)) {
            std::cerr << "Another instance is already running for this directory (PID " << pid
                      << ")\n";
            return 1;
        }
        std::cerr << "Stale lock file found. Removing and continuing...\n";
        procutil::release_lock_file(lock_path);
        lock.locked = procutil::acquire_lock_file(lock_path);
        if (!lock.locked) {
            std::cerr << "Failed to acquire lock." << std::endl;
            return 1;
        }
    }
    setup_environment(opts);
    setup_logging(opts);
    fs::create_directories(opts.log_dir);
    std::vector<fs::path> all_repos;
    std::map<fs::path, RepoInfo> repo_infos;
    prepare_repos(opts, all_repos, repo_infos);
    std::set<fs::path> skip_repos;
    std::mutex mtx;
    std::atomic<bool> scanning(false);
    std::atomic<bool> running(true);
    std::string current_action = "Idle";
    std::mutex action_mtx;
    g_running_ptr = &running;
    std::signal(SIGINT, handle_signal);
#ifndef _WIN32
    std::signal(SIGTERM, handle_signal);
#endif
    std::jthread scan_thread;
    std::chrono::milliseconds countdown_ms(0);
    std::chrono::milliseconds cli_countdown_ms(0);
    std::unique_ptr<AltScreenGuard> guard;
    if (!opts.cli && !opts.silent)
        guard = std::make_unique<AltScreenGuard>();
    size_t concurrency = opts.concurrency;
    if (opts.max_threads > 0 && concurrency > opts.max_threads)
        concurrency = opts.max_threads;
#ifndef _WIN32
    int status_fd = -1;
    std::vector<int> status_clients;
    if (!opts.attach_name.empty()) {
        status_fd = procutil::create_status_socket(opts.attach_name);
        if (status_fd >= 0) {
            int fl = fcntl(status_fd, F_GETFL, 0);
            fcntl(status_fd, F_SETFL, fl | O_NONBLOCK);
        }
    }
#endif
    while (running) {
#ifndef _WIN32
        if (status_fd >= 0) {
            int c = accept(status_fd, nullptr, nullptr);
            if (c >= 0) {
                int fl = fcntl(c, F_GETFL, 0);
                fcntl(c, F_SETFL, fl | O_NONBLOCK);
                status_clients.push_back(c);
            }
        }
#endif
        if (!scanning && scan_thread.joinable()) {
            scan_thread.join();
            git_libgit2_shutdown();
            git_libgit2_init();
            if (opts.single_run)
                running = false;
        }
        if (running && countdown_ms <= std::chrono::milliseconds(0) && !scanning) {
            {
                std::lock_guard<std::mutex> lk(mtx);
                for (auto& [p, info] : repo_infos) {
                    if (info.status == RS_PULLING || info.status == RS_CHECKING) {
                        if (!opts.silent)
                            std::cerr << "Manually clearing stale busy state for " << p << "\n";
                        info.status = RS_PENDING;
                        info.message = "Pending...";
                    }
                }
            }
            scanning = true;
            scan_thread = std::jthread(
                scan_repos, std::cref(all_repos), std::ref(repo_infos), std::ref(skip_repos),
                std::ref(mtx), std::ref(scanning), std::ref(running), std::ref(current_action),
                std::ref(action_mtx), opts.include_private, std::cref(opts.log_dir),
                opts.check_only, opts.hash_check, concurrency, opts.cpu_percent_limit,
                opts.mem_limit, opts.download_limit, opts.upload_limit, opts.disk_limit,
                opts.silent, opts.force_pull);
            countdown_ms = std::chrono::seconds(opts.interval);
        }
#ifndef _WIN32
        std::string status_msg;
#endif
        {
            std::lock_guard<std::mutex> lk(mtx);
            int sec_left =
                (int)std::chrono::duration_cast<std::chrono::seconds>(countdown_ms).count();
            if (sec_left < 0)
                sec_left = 0;
            std::string act;
            {
                std::lock_guard<std::mutex> a_lk(action_mtx);
                act = current_action;
            }
#ifndef _WIN32
            status_msg = act + "\n";
#endif
            update_ui(opts, all_repos, repo_infos, sec_left, scanning, act, cli_countdown_ms);
        }
#ifndef _WIN32
        if (status_fd >= 0) {
            const std::string& msg = status_msg;
            for (auto it = status_clients.begin(); it != status_clients.end();) {
                ssize_t w = send(*it, msg.c_str(), msg.size(), MSG_NOSIGNAL);
                if (w <= 0) {
                    close(*it);
                    it = status_clients.erase(it);
                } else {
                    ++it;
                }
            }
        }
#endif
        std::this_thread::sleep_for(opts.refresh_ms);
        countdown_ms -= opts.refresh_ms;
        cli_countdown_ms -= opts.refresh_ms;
    }
    running = false;
    if (scan_thread.joinable()) {
        scan_thread.join();
        git_libgit2_shutdown();
        git_libgit2_init();
    }
    if (logger_initialized())
        log_info("Program exiting");
    shutdown_logger();
#ifndef _WIN32
    if (status_fd >= 0) {
        for (int c : status_clients)
            close(c);
        procutil::remove_status_socket(opts.attach_name, status_fd);
    }
#endif
    return 0;
}

#ifndef AUTOGITPULL_NO_MAIN
int main(int argc, char* argv[]) {
    git::GitInitGuard git_guard;
    try {
        Options opts = parse_options(argc, argv);
        if (opts.show_help) {
            print_help(argv[0]);
            return 0;
        }
        if (opts.print_version) {
            std::cout << AUTOGITPULL_VERSION << "\n";
            return 0;
        }
        if (opts.remove_lock) {
            if (!opts.root.empty()) {
                fs::path lock = opts.root / ".autogitpull.lock";
                procutil::release_lock_file(lock);
            }
            return 0;
        }
        return run_event_loop(opts);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
#endif // AUTOGITPULL_NO_MAIN
