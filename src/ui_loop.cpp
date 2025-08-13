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
#include "help_text.hpp"
#include "lock_utils.hpp"
#include "linux_daemon.hpp"
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#else
#include <conio.h>
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

bool poll_timed_out(const Options& opts, std::chrono::steady_clock::time_point start,
                    std::chrono::steady_clock::time_point now) {
    return opts.exit_on_timeout && opts.pull_timeout.count() > 0 && now - start > opts.pull_timeout;
}

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
    case RS_NOT_GIT:
        return "NotGit";
    case RS_HEAD_PROBLEM:
        return "HEAD/BR";
    case RS_DIRTY:
        return "Dirty";
    case RS_TIMEOUT:
        return "TimedOut";
    case RS_RATE_LIMIT:
        return "RateLimit";
    case RS_REMOTE_AHEAD:
        return "RemoteUp";
    case RS_TEMPFAIL:
        return "TempFail";
    }
    return "";
}
void draw_cli(const std::vector<fs::path>& all_repos,
              const std::map<fs::path, RepoInfo>& repo_infos, int seconds_left, bool scanning,
              const std::string& action, bool show_skipped, bool show_notgit, int runtime_sec,
              bool show_repo_count, bool session_dates_only, bool censor_names, char censor_char) {
    if (show_repo_count) {
        size_t active = 0;
        for (const auto& p : all_repos) {
            auto it = repo_infos.find(p);
            RepoStatus st = it != repo_infos.end() ? it->second.status : RS_PENDING;
            if (st != RS_SKIPPED && st != RS_NOT_GIT)
                ++active;
        }
        std::cout << "Repos: " << active << "/" << all_repos.size() << "\n";
    }
    std::cout << "Status: ";
    if (scanning || action != "Idle")
        std::cout << action;
    else
        std::cout << "Idle";
    std::cout << " - Next scan in " << seconds_left << "s";
    if (runtime_sec >= 0)
        std::cout << " - Runtime " << format_duration_short(std::chrono::seconds(runtime_sec));
    std::cout << "\n";
    for (const auto& p : all_repos) {
        RepoInfo ri;
        auto it = repo_infos.find(p);
        if (it != repo_infos.end())
            ri = it->second;
        else
            ri = RepoInfo{p, RS_PENDING, "Pending...", "", "", "", "", "", 0, false};
        if ((ri.status == RS_SKIPPED && !show_skipped) || (ri.status == RS_NOT_GIT && !show_notgit))
            continue;
        std::string name = p.filename().string();
        if (censor_names)
            name.assign(name.size(), censor_char);
        std::cout << " [" << status_label(ri.status) << "] " << name;
        if (!ri.branch.empty()) {
            std::cout << " (" << ri.branch;
            if (!ri.commit.empty())
                std::cout << "@" << ri.commit;
            std::cout << ")";
        }
        if ((!session_dates_only || ri.pulled) &&
            (!ri.commit_author.empty() || !ri.commit_date.empty())) {
            std::cout << " {";
            bool first = true;
            if (!ri.commit_author.empty()) {
                std::cout << ri.commit_author;
                first = false;
            }
            if (!ri.commit_date.empty()) {
                if (!first)
                    std::cout << ' ';
                std::cout << ri.commit_date;
            }
            std::cout << "}";
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
        init_logger(opts.log_file, opts.log_level, opts.max_log_size);
        if (logger_initialized())
            log_info("Program started");
    }
    if (opts.use_syslog)
        init_syslog(opts.syslog_facility);
}

// Build repository list and populate info table
static void prepare_repos(const Options& opts, std::vector<fs::path>& all_repos,
                          std::map<fs::path, RepoInfo>& repo_infos) {
    if (opts.single_repo) {
        all_repos = {opts.root};
    } else {
        std::vector<fs::path> roots{opts.root};
        roots.insert(roots.end(), opts.include_dirs.begin(), opts.include_dirs.end());
        all_repos = build_repo_list(roots, opts.recursive_scan, opts.ignore_dirs, opts.max_depth);
        if (opts.sort_mode == Options::ALPHA)
            std::sort(all_repos.begin(), all_repos.end());
        else if (opts.sort_mode == Options::REVERSE)
            std::sort(all_repos.rbegin(), all_repos.rend());
    }
    for (const auto& p : all_repos)
        repo_infos[p] = RepoInfo{p, RS_PENDING, "Pending...", "", "", "", "", "", 0, false};
}

// Render either the TUI or CLI output
static void update_ui(const Options& opts, const std::vector<fs::path>& all_repos,
                      const std::map<fs::path, RepoInfo>& repo_infos, int interval, int sec_left,
                      bool scanning, const std::string& act,
                      std::chrono::milliseconds& cli_countdown_ms, const std::string& message,
                      int runtime_sec) {
    if (!opts.silent && !opts.cli) {
        draw_tui(all_repos, repo_infos, interval, sec_left, scanning, act, opts.show_skipped,
                 opts.show_notgit, opts.show_version, opts.cpu_tracker, opts.mem_tracker,
                 opts.thread_tracker, opts.net_tracker, opts.cpu_core_mask != 0, opts.show_vmem,
                 opts.show_commit_date, opts.show_commit_author, opts.session_dates_only,
                 opts.no_colors, opts.custom_color, message, runtime_sec, opts.show_datetime_line,
                 opts.show_header, opts.show_repo_count, opts.censor_names, opts.censor_char);
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
    std::unique_ptr<procutil::LockFileGuard> lock;
    fs::path lock_path = opts.root / ".autogitpull.lock";
    if (!opts.ignore_lock) {
        lock = std::make_unique<procutil::LockFileGuard>(lock_path);
        if (!lock->locked) {
            unsigned long pid = 0;
            if (procutil::read_lock_pid(lock_path, pid) && procutil::process_running(pid)) {
                std::cerr << "Another instance is already running for this directory (PID " << pid
                          << ")\n";
                return 1;
            }
            std::cerr << "Stale lock file found. Removing and continuing...\n";
            procutil::release_lock_file(lock_path);
            lock->locked = procutil::acquire_lock_file(lock_path);
            if (!lock->locked) {
                std::cerr << "Failed to acquire lock." << std::endl;
                return 1;
            }
        }
    }
    setup_environment(opts);
    procutil::get_cpu_percent();
    procutil::get_memory_usage_mb();
    procutil::get_thread_count();
    setup_logging(opts);
    int interval = opts.interval;
    if (!opts.log_dir.empty())
        fs::create_directories(opts.log_dir);
    std::vector<fs::path> all_repos;
    std::map<fs::path, RepoInfo> repo_infos;
    std::set<fs::path> first_validated;
    prepare_repos(opts, all_repos, repo_infos);
    size_t valid_count = 0;
    for (const auto& p : all_repos) {
        if (fs::is_directory(p) && git::is_git_repo(p)) {
            ++valid_count;
        }
    }
    int empty_attempts = 0;
    while (valid_count == 0 && opts.wait_empty &&
           (opts.wait_empty_limit == 0 || empty_attempts < opts.wait_empty_limit)) {
        if (!opts.silent)
            std::cout << "No valid repositories found. Retrying in " << interval << "s..."
                      << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(interval));
        prepare_repos(opts, all_repos, repo_infos);
        valid_count = 0;
        for (const auto& p : all_repos) {
            if (fs::is_directory(p) && git::is_git_repo(p))
                ++valid_count;
        }
        ++empty_attempts;
    }
    if (valid_count == 0) {
        std::cout << "No valid repositories found. Exiting." << std::endl;
        return 0;
    }
    if (opts.cli && !opts.silent) {
        std::cout << "Interval: " << interval << "s"
                  << " Refresh: " << opts.refresh_ms.count() << "ms";
        if (opts.pull_timeout.count() > 0)
            std::cout << " Timeout: " << opts.pull_timeout.count() << "s";
        std::cout << " SkipTimeouts: " << (opts.skip_timeout ? "yes" : "no");
        if (opts.keep_first_valid)
            std::cout << " KeepFirst: yes";
        if (opts.runtime_limit.count() > 0)
            std::cout << " Runtime limit: " << format_duration_short(opts.runtime_limit);
        if (opts.rescan_new)
            std::cout << " Rescan: " << opts.rescan_interval.count() << "m";
        std::cout << std::endl;
    }
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
    th_compat::jthread scan_thread;
    std::chrono::steady_clock::time_point scan_start;
    std::chrono::milliseconds countdown_ms(0);
    std::chrono::milliseconds cli_countdown_ms(0);
    std::chrono::milliseconds rescan_countdown_ms(opts.rescan_new ? opts.rescan_interval
                                                                  : std::chrono::milliseconds(0));
    bool first_cycle = true;
    std::unique_ptr<AltScreenGuard> guard;
    if (!opts.cli && !opts.silent)
        guard = std::make_unique<AltScreenGuard>();
    std::string user_message;
    bool confirm_quit = false;
    std::chrono::steady_clock::time_point confirm_until;
    std::string confirm_prev_action;
#ifndef _WIN32
    struct TermGuard {
        termios orig;
        int orig_fl = 0;
        bool active = false;
        void setup() {
            if (tcgetattr(STDIN_FILENO, &orig) == 0) {
                termios t = orig;
                t.c_lflag &= ~(ICANON | ECHO);
                tcsetattr(STDIN_FILENO, TCSANOW, &t);
                orig_fl = fcntl(STDIN_FILENO, F_GETFL, 0);
                fcntl(STDIN_FILENO, F_SETFL, orig_fl | O_NONBLOCK);
                active = true;
            }
        }
        ~TermGuard() {
            if (active) {
                tcsetattr(STDIN_FILENO, TCSANOW, &orig);
                fcntl(STDIN_FILENO, F_SETFL, orig_fl);
            }
        }
    } term_guard;
#else
    struct TermGuard {
        void setup() {}
    } term_guard;
#endif
    if (opts.enable_hotkeys && !opts.cli && !opts.silent)
        term_guard.setup();
    auto start_time = std::chrono::steady_clock::now();
    auto last_loop = start_time;
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
        auto now = std::chrono::steady_clock::now();
        if (scanning && poll_timed_out(opts, scan_start, now)) {
            log_error("Polling exceeded timeout; terminating worker");
            running = false;
#if defined(__cpp_lib_jthread)
            scan_thread.request_stop();
#endif
            break;
        }
        if (now - last_loop > std::chrono::minutes(10)) {
            log_info("Detected long pause; resuming");
            if (opts.kill_on_sleep) {
                log_info("Exiting due to system sleep");
                break;
            }
            countdown_ms = std::chrono::milliseconds(0);
        }
        last_loop = now;
        auto elapsed = now - start_time;
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
            if (first_cycle) {
                if (opts.keep_first_valid) {
                    for (const auto& [p, info] : repo_infos) {
                        if (info.status != RS_SKIPPED && info.status != RS_ERROR &&
                            info.status != RS_NOT_GIT)
                            first_validated.insert(p);
                    }
                }
                if (opts.cli && !opts.silent && opts.cli_print_skipped && !opts.show_skipped) {
                    for (const auto& sp : skip_repos) {
                        std::string name = sp.filename().string();
                        if (opts.censor_names)
                            name.assign(name.size(), opts.censor_char);
                        std::cout << "Skipped " << name << "\n";
                    }
                }
                first_cycle = false;
            }
            if (opts.single_run)
                running = false;
        }
        if (running && countdown_ms <= std::chrono::milliseconds(0) && !scanning) {
            if (opts.rescan_new && rescan_countdown_ms <= std::chrono::milliseconds(0)) {
                std::vector<fs::path> roots{opts.root};
                roots.insert(roots.end(), opts.include_dirs.begin(), opts.include_dirs.end());
                auto new_repos =
                    build_repo_list(roots, opts.recursive_scan, opts.ignore_dirs, opts.max_depth);
                if (opts.keep_first_valid) {
                    for (const auto& p : first_validated) {
                        if (std::find(new_repos.begin(), new_repos.end(), p) == new_repos.end())
                            new_repos.push_back(p);
                    }
                }
                if (opts.sort_mode == Options::ALPHA)
                    std::sort(new_repos.begin(), new_repos.end());
                else if (opts.sort_mode == Options::REVERSE)
                    std::sort(new_repos.rbegin(), new_repos.rend());
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    for (const auto& p : new_repos) {
                        if (!repo_infos.count(p))
                            repo_infos[p] =
                                RepoInfo{p, RS_PENDING, "Pending...", "", "", "", "", "", 0, false};
                    }
                }
                for (const auto& p : new_repos) {
                    if (std::find(all_repos.begin(), all_repos.end(), p) == all_repos.end())
                        all_repos.push_back(p);
                }
                if (opts.sort_mode == Options::ALPHA)
                    std::sort(all_repos.begin(), all_repos.end());
                else if (opts.sort_mode == Options::REVERSE)
                    std::sort(all_repos.rbegin(), all_repos.rend());
                rescan_countdown_ms = opts.rescan_interval;
            }
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
            scan_start = std::chrono::steady_clock::now();
            scan_thread = th_compat::jthread(
                scan_repos, std::cref(all_repos), std::ref(repo_infos), std::ref(skip_repos),
                std::ref(mtx), std::ref(scanning), std::ref(running), std::ref(current_action),
                std::ref(action_mtx), opts.include_private, std::cref(opts.log_dir),
                opts.check_only, opts.hash_check, concurrency, opts.cpu_percent_limit,
                opts.mem_limit, opts.download_limit, opts.upload_limit, opts.disk_limit,
                opts.silent, opts.cli, opts.force_pull, opts.skip_timeout,
                opts.skip_accessible_errors, opts.updated_since, opts.show_pull_author,
                opts.pull_timeout, opts.retry_skipped, opts.repo_overrides);
            countdown_ms = std::chrono::seconds(interval);
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
            update_ui(opts, all_repos, repo_infos, interval, sec_left, scanning, act,
                      cli_countdown_ms, user_message, opts.show_runtime ? runtime_sec : -1);
        }
#if defined(_WIN32)
        if (opts.enable_hotkeys && !opts.cli && _kbhit()) {
            int ch = _getch();
            if (ch != 0 && ch != 224) {
                char c = static_cast<char>(ch);
                if (c == 'r') {
                    countdown_ms = std::chrono::milliseconds(0);
                    user_message = "Scanning now";
                } else if (c == 'n') {
                    rescan_countdown_ms = std::chrono::milliseconds(0);
                    countdown_ms = std::chrono::milliseconds(0);
                    user_message = "Rescanning repos";
                } else if (c == 'p') {
                    interval += 10;
                    countdown_ms = std::chrono::seconds(interval);
                    user_message = "Interval " + std::to_string(interval) + "s";
                } else if (c == 'o') {
                    if (interval > 10)
                        interval -= 10;
                    countdown_ms = std::chrono::seconds(interval);
                    user_message = "Interval " + std::to_string(interval) + "s";
                } else if (c == 'q') {
                    if (!confirm_quit) {
                        confirm_quit = true;
                        confirm_until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                        {
                            std::lock_guard<std::mutex> lk(action_mtx);
                            confirm_prev_action = current_action;
                            current_action = "Press q again to quit";
                        }
                    } else {
                        running = false;
                    }
                } else {
                    if (confirm_quit) {
                        confirm_quit = false;
                        std::lock_guard<std::mutex> lk(action_mtx);
                        current_action = confirm_prev_action;
                    }
                }
            }
        }
#else
        if (opts.enable_hotkeys && !opts.cli && !opts.silent) {
            int ch = getchar();
            if (ch != EOF) {
                char c = static_cast<char>(ch);
                if (c == 'r') {
                    countdown_ms = std::chrono::milliseconds(0);
                    user_message = "Scanning now";
                } else if (c == 'n') {
                    rescan_countdown_ms = std::chrono::milliseconds(0);
                    countdown_ms = std::chrono::milliseconds(0);
                    user_message = "Rescanning repos";
                } else if (c == 'p') {
                    interval += 10;
                    countdown_ms = std::chrono::seconds(interval);
                    user_message = "Interval " + std::to_string(interval) + "s";
                } else if (c == 'o') {
                    if (interval > 10)
                        interval -= 10;
                    countdown_ms = std::chrono::seconds(interval);
                    user_message = "Interval " + std::to_string(interval) + "s";
                } else if (c == 'q') {
                    if (!confirm_quit) {
                        confirm_quit = true;
                        confirm_until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                        {
                            std::lock_guard<std::mutex> lk(action_mtx);
                            confirm_prev_action = current_action;
                            current_action = "Press q again to quit";
                        }
                    } else {
                        running = false;
                    }
                } else {
                    if (confirm_quit) {
                        confirm_quit = false;
                        std::lock_guard<std::mutex> lk(action_mtx);
                        current_action = confirm_prev_action;
                    }
                }
            }
        }
#endif
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
        if (opts.rescan_new)
            rescan_countdown_ms -= opts.refresh_ms;
        if (confirm_quit && std::chrono::steady_clock::now() > confirm_until) {
            confirm_quit = false;
            std::lock_guard<std::mutex> lk(action_mtx);
            current_action = confirm_prev_action;
        }
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
