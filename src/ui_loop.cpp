#include "ui_loop.hpp"

#include <csignal>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <sstream>
#include <cctype>
#include <memory>

#include "scanner.hpp"
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
        {"--attach", "-A", "<name>", "Attach to daemon and show status", "General"},
        {"--background", "-b", "<name>", "Run in background with attach name", "General"},
        {"--reattach", "-B", "<name>", "Reattach to background process", "General"},
        {"--show-runtime", "", "", "Display elapsed runtime", "General"},
        {"--max-runtime", "", "<sec>", "Exit after given runtime", "General"},
        {"--check-only", "", "", "Only check for updates", "Actions"},
        {"--no-hash-check", "", "", "Always pull without hash check", "Actions"},
        {"--force-pull", "", "", "Discard local changes when pulling", "Actions"},
        {"--discard-dirty", "", "", "Alias for --force-pull", "Actions"},
        {"--install-daemon", "", "", "Install background daemon", "Actions"},
        {"--uninstall-daemon", "", "", "Uninstall background daemon", "Actions"},
        {"--daemon-config", "", "<file>", "Config file for daemon install", "Actions"},
        {"--install-service", "", "", "Install system service", "Actions"},
        {"--uninstall-service", "", "", "Uninstall system service", "Actions"},
        {"--service-config", "", "<file>", "Config file for service install", "Actions"},
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
              const std::string& action, bool show_skipped, int runtime_sec) {
    std::cout << "Status: ";
    if (scanning)
        std::cout << action;
    else
        std::cout << "Idle";
    std::cout << " - Next scan in " << seconds_left << "s";
    if (runtime_sec >= 0)
        std::cout << " - Runtime " << runtime_sec << "s";
    std::cout << "\n";
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
                      const std::string& act, std::chrono::milliseconds& cli_countdown_ms,
                      int runtime_sec) {
    if (!opts.silent && !opts.cli) {
        draw_tui(all_repos, repo_infos, opts.interval, sec_left, scanning, act, opts.show_skipped,
                 opts.show_version, opts.cpu_tracker, opts.mem_tracker, opts.thread_tracker,
                 opts.net_tracker, opts.cpu_core_mask != 0, runtime_sec);
    } else if (!opts.silent && opts.cli && cli_countdown_ms <= std::chrono::milliseconds(0)) {
        draw_cli(all_repos, repo_infos, sec_left, scanning, act, opts.show_skipped, runtime_sec);
        cli_countdown_ms = opts.refresh_ms;
    }
}
int run_event_loop(const Options& opts) {
    debugMemory = opts.debug_memory;
    dumpState = opts.dump_state;
    dumpThreshold = opts.dump_threshold;
#ifndef _WIN32
    if (opts.reattach) {
        int fd = procutil::connect_status_socket(opts.attach_name);
        if (fd < 0) {
            std::cerr << "Failed to connect to background" << std::endl;
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
    if (opts.run_background) {
        if (!procutil::daemonize()) {
            std::cerr << "Failed to daemonize" << std::endl;
            return 1;
        }
    }
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
    if (!opts.log_dir.empty())
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
    auto start_time = std::chrono::steady_clock::now();
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
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        int runtime_sec =
            static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
        if (opts.runtime_limit.count() > 0 && elapsed >= opts.runtime_limit)
            running = false;
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
            update_ui(opts, all_repos, repo_infos, sec_left, scanning, act, cli_countdown_ms,
                      opts.show_runtime ? runtime_sec : -1);
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
