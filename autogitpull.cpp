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
#include "arg_parser.hpp"
#include "git_utils.hpp"
#include "tui.hpp"
#include "repo.hpp"
#include "logger.hpp"

namespace fs = std::filesystem;

struct AltScreenGuard {
    AltScreenGuard() { enable_win_ansi(); std::cout << "\033[?1049h"; }
    ~AltScreenGuard() { std::cout << "\033[?1049l" << std::flush; }
};

std::atomic<bool>* g_running_ptr = nullptr;

void handle_signal(int) {
    if (g_running_ptr) g_running_ptr->store(false);
}


// Process a single repository
void process_repo(const fs::path& p,
                  std::map<fs::path, RepoInfo>& repo_infos,
                  std::set<fs::path>& skip_repos,
                  std::mutex& mtx,
                  std::atomic<bool>& running,
                  std::string& action,
                  std::mutex& action_mtx,
                  bool include_private,
                  const fs::path& log_dir,
                  bool check_only,
                  bool hash_check) {
    if (!running) return;
    if (logger_initialized())
        log_debug("Checking repo " + p.string());
    {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = repo_infos.find(p);
        if (it != repo_infos.end() &&
            (it->second.status == RS_PULLING || it->second.status == RS_CHECKING)) {
            // Repo already being processed elsewhere
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
    if (!fs::exists(p)) {
        ri.status = RS_ERROR;
        ri.message = "Missing";
        if (logger_initialized())
            log_error(p.string() + " missing");
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
        return;
    }
    if (skip_repos.count(p)) {
        ri.status = RS_SKIPPED;
        ri.message = "Skipped after fatal error";
        if (logger_initialized())
            log_warning(p.string() + " skipped after fatal error");
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
        return;
    }
    try {
        ri.status = RS_CHECKING;
        ri.message = "";
        if (fs::is_directory(p) && git::is_git_repo(p)) {
            ri.commit = git::get_local_hash(p);
            if (ri.commit.size() > 7) ri.commit = ri.commit.substr(0,7);
            std::string origin = git::get_origin_url(p);
            if (!include_private) {
                if (!git::is_github_url(origin)) {
                    ri.status = RS_SKIPPED;
                    ri.message = "Non-GitHub repo (skipped)";
                    if (logger_initialized())
                        log_debug(p.string() + " skipped: non-GitHub repo");
                    std::lock_guard<std::mutex> lk(mtx);
                    repo_infos[p] = ri;
                    return;
                }
                if (!git::remote_accessible(p)) {
                    ri.status = RS_SKIPPED;
                    ri.message = "Private or inaccessible repo";
                    if (logger_initialized())
                        log_debug(p.string() + " skipped: private or inaccessible");
                    std::lock_guard<std::mutex> lk(mtx);
                    repo_infos[p] = ri;
                    return;
                }
            }
            ri.branch = git::get_current_branch(p);
            if (ri.branch.empty() || ri.branch == "HEAD") {
                ri.status = RS_HEAD_PROBLEM;
                ri.message = "Detached HEAD or branch error";
                skip_repos.insert(p);
            } else {
                bool needs_pull = true;
                if (hash_check) {
                    std::string local = git::get_local_hash(p);
                    bool auth_fail = false;
                    std::string remote = git::get_remote_hash(p, ri.branch,
                                                               include_private,
                                                               &auth_fail);
                    ri.auth_failed = auth_fail;
                    if (local.empty() || remote.empty()) {
                        ri.status = RS_ERROR;
                        ri.message = "Error getting hashes or remote";
                        skip_repos.insert(p);
                        needs_pull = false;
                    } else if (local == remote) {
                        ri.status = RS_UP_TO_DATE;
                        ri.message = "Up to date";
                        ri.commit = local.substr(0,7);
                        needs_pull = false;
                    }
                }
                if (needs_pull) {
                    if (check_only) {
                        ri.status = RS_REMOTE_AHEAD;
                        ri.message = hash_check ? "Remote ahead" : "Update possible";
                        ri.commit = git::get_local_hash(p);
                        if (ri.commit.size() > 7) ri.commit = ri.commit.substr(0,7);
                        if (logger_initialized())
                            log_debug(p.string() + " remote ahead");
                    } else {
                        ri.status = RS_PULLING;
                        ri.message = "Remote ahead, pulling...";
                        ri.progress = 0;
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
                        int code = git::try_pull(p, pull_log, &progress_cb,
                                                 include_private,
                                                 &pull_auth_fail);
                        ri.auth_failed = pull_auth_fail;
                        ri.last_pull_log = pull_log;
                        fs::path log_file_path;
                        if (!log_dir.empty()) {
                            std::string ts = timestamp();
                            for (char& ch : ts) {
                                if (ch == ' ' || ch == ':') ch = '_';
                                else if (ch == '/') ch = '-';
                            }
                            log_file_path = log_dir / (p.filename().string() + "_" + ts + ".log");
                            std::ofstream ofs(log_file_path);
                            ofs << pull_log;
                        }
                        if (code == 0) {
                            ri.status = RS_PULL_OK;
                            ri.message = "Pulled successfully";
                            ri.commit = git::get_local_hash(p);
                            if (ri.commit.size() > 7) ri.commit = ri.commit.substr(0,7);
                            if (logger_initialized())
                                log_info(p.string() + " pulled successfully");
                        } else if (code == 1) {
                            ri.status = RS_PKGLOCK_FIXED;
                            ri.message = "package-lock.json auto-reset & pulled";
                            ri.commit = git::get_local_hash(p);
                            if (ri.commit.size() > 7) ri.commit = ri.commit.substr(0,7);
                            if (logger_initialized())
                                log_info(p.string() + " package-lock reset and pulled");
                        } else {
                            ri.status = RS_ERROR;
                            ri.message = "Pull failed (see log)";
                            skip_repos.insert(p);
                            if (logger_initialized())
                                log_error(p.string() + " pull failed");
                        }
                        if (!log_file_path.empty()) {
                            ri.message += " - " + log_file_path.string();
                        }
                    }
                }
            }
        } else {
            ri.status = RS_SKIPPED;
            ri.message = "Not a git repo (skipped)";
            skip_repos.insert(p);
            if (logger_initialized())
                log_debug(p.string() + " skipped: not a git repo");
        }
    } catch (const fs::filesystem_error& e) {
        ri.status = RS_ERROR;
        ri.message = e.what();
        skip_repos.insert(p);
        if (logger_initialized())
            log_error(p.string() + " error: " + ri.message);
    }
    std::lock_guard<std::mutex> lk(mtx);
    repo_infos[p] = ri;
    if (logger_initialized())
        log_debug(p.string() + " -> " + ri.message);
}

// Background thread: scan and update info
void scan_repos(
    const std::vector<fs::path>& all_repos,
    std::map<fs::path, RepoInfo>& repo_infos,
    std::set<fs::path>& skip_repos,
    std::mutex& mtx,
    std::atomic<bool>& scanning_flag,
    std::atomic<bool>& running,
    std::string& action,
    std::mutex& action_mtx,
    bool include_private,
    const fs::path& log_dir,
    bool check_only,
    bool hash_check,
    size_t concurrency
) {
    if (concurrency == 0) concurrency = 1;
    concurrency = std::min(concurrency, all_repos.size());
    if (logger_initialized())
        log_debug("Scanning repositories");

    std::atomic<size_t> next_index{0};
    auto worker = [&]() {
        while (running) {
            size_t idx = next_index.fetch_add(1);
            if (idx >= all_repos.size()) break;
            const auto& p = all_repos[idx];
            process_repo(p, repo_infos, skip_repos, mtx, running,
                         action, action_mtx,
                         include_private, log_dir, check_only, hash_check);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(concurrency);
    for (size_t i = 0; i < concurrency; ++i)
        threads.emplace_back(worker);
    for (auto& t : threads) t.join();
    scanning_flag = false;
    {
        std::lock_guard<std::mutex> lk(action_mtx);
        action = "Idle";
    }
    if (logger_initialized())
        log_debug("Scan complete");
}

int main(int argc, char* argv[]) {
    git::GitInitGuard git_guard;
    try {
        const std::set<std::string> known{ "--include-private", "--show-skipped", "--interval", "--refresh-rate", "--log-dir", "--log-file", "--concurrency", "--check-only", "--no-hash-check", "--log-level", "--verbose", "--help" };
        ArgParser parser(argc, argv, known);

        if (parser.has_flag("--help")) {
            std::cout << "Usage: " << argv[0]
                      << " <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--refresh-rate <ms>] [--log-dir <path>] [--log-file <path>] [--log-level <level>] [--verbose] [--concurrency <n>] [--check-only] [--no-hash-check] [--help]\n";
            return 0;
        }

        if (parser.positional().size() != 1) {
            std::cerr << "Usage: " << argv[0]
                      << " <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--refresh-rate <ms>] [--log-dir <path>] [--log-file <path>] [--log-level <level>] [--verbose] [--concurrency <n>] [--check-only] [--no-hash-check] [--help]\n";
            return 1;
        }

        if (!parser.unknown_flags().empty()) {
            for (const auto& f : parser.unknown_flags()) {
                std::cerr << "Unknown option: " << f << "\n";
            }
            return 1;
        }
        bool include_private = parser.has_flag("--include-private");
        bool show_skipped = parser.has_flag("--show-skipped");
        bool check_only = parser.has_flag("--check-only");
        bool hash_check = !parser.has_flag("--no-hash-check");
        LogLevel log_level = LogLevel::INFO;
        if (parser.has_flag("--verbose")) {
            log_level = LogLevel::DEBUG;
        }
        if (parser.has_flag("--log-level")) {
            std::string val = parser.get_option("--log-level");
            if (val.empty()) {
                std::cerr << "--log-level requires a value\n";
                return 1;
            }
            for (auto& c : val) c = toupper(static_cast<unsigned char>(c));
            if (val == "DEBUG") log_level = LogLevel::DEBUG;
            else if (val == "INFO") log_level = LogLevel::INFO;
            else if (val == "WARNING" || val == "WARN") log_level = LogLevel::WARNING;
            else if (val == "ERROR") log_level = LogLevel::ERROR;
            else {
                std::cerr << "Invalid log level: " << val << "\n";
                return 1;
            }
        }
        size_t concurrency = 3;
        int interval = 30;
        std::chrono::milliseconds refresh_ms(250);
        if (parser.has_flag("--interval")) {
            std::string val = parser.get_option("--interval");
            if (val.empty()) {
                std::cerr << "--interval requires a value in seconds\n";
                return 1;
            }
            try {
                interval = std::stoi(val);
            } catch (...) {
                std::cerr << "Invalid value for --interval: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--refresh-rate")) {
            std::string val = parser.get_option("--refresh-rate");
            if (val.empty()) {
                std::cerr << "--refresh-rate requires a value in milliseconds\n";
                return 1;
            }
            try {
                refresh_ms = std::chrono::milliseconds(std::stoi(val));
            } catch (...) {
                std::cerr << "Invalid value for --refresh-rate: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--concurrency")) {
            std::string val = parser.get_option("--concurrency");
            if (val.empty()) {
                std::cerr << "--concurrency requires a numeric value\n";
                return 1;
            }
            try {
                concurrency = static_cast<size_t>(std::stoul(val));
                if (concurrency == 0) concurrency = 1;
            } catch (...) {
                std::cerr << "Invalid value for --concurrency: " << val << "\n";
                return 1;
            }
        }
        fs::path log_dir;
        if (parser.has_flag("--log-dir")) {
            std::string val = parser.get_option("--log-dir");
            if (val.empty()) {
                std::cerr << "--log-dir requires a path\n";
                return 1;
            }
            log_dir = val;
            try {
                if (fs::exists(log_dir) && !fs::is_directory(log_dir)) {
                    std::cerr << "--log-dir path exists and is not a directory\n";
                    return 1;
                }
                fs::create_directories(log_dir);
            } catch (const std::exception& e) {
                std::cerr << "Failed to create log directory: " << e.what() << "\n";
                return 1;
            }
        }
        std::string log_file;
        if (parser.has_flag("--log-file")) {
            std::string val = parser.get_option("--log-file");
            if (val.empty()) {
                std::cerr << "--log-file requires a path\n";
                return 1;
            }
            log_file = val;
        }
        fs::path root = parser.positional().front();
        if (!fs::exists(root) || !fs::is_directory(root)) {
            std::cerr << "Root path does not exist or is not a directory.\n";
            return 1;
        }

        if (!log_file.empty()) {
            init_logger(log_file, log_level);
            if (logger_initialized())
                log_info("Program started");
            else
                std::cerr << "Failed to open log file: " << log_file << "\n";
        }

        // Grab all first-level subdirs at startup (fixed list)
        std::vector<fs::path> all_repos;
        for (const auto& entry : fs::directory_iterator(root)) {
            all_repos.push_back(entry.path());
        }
        std::map<fs::path, RepoInfo> repo_infos;
        for (const auto& p : all_repos) {
            repo_infos[p] = RepoInfo{p, RS_PENDING, "Pending...", "", "", "", 0, false};
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

        std::thread scan_thread;
        std::chrono::milliseconds countdown_ms(0); // Run immediately on start

        AltScreenGuard guard;

        while (running) {
            if (!scanning && scan_thread.joinable())
                scan_thread.join();

            if (running && countdown_ms <= std::chrono::milliseconds(0) && !scanning) {
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    for (auto& [p, info] : repo_infos) {
                        if (info.status == RS_PULLING || info.status == RS_CHECKING) {
                            std::cerr << "Manually clearing stale busy state for "
                                      << p << "\n";
                            info.status = RS_PENDING;
                            info.message = "Pending...";
                        }
                    }
                }
                scanning = true;
                scan_thread = std::thread(scan_repos, std::cref(all_repos), std::ref(repo_infos),
                                          std::ref(skip_repos), std::ref(mtx),
                                          std::ref(scanning), std::ref(running),
                                          std::ref(current_action), std::ref(action_mtx),
                                          include_private,
                                          std::cref(log_dir), check_only, hash_check, concurrency);
                countdown_ms = std::chrono::seconds(interval);
            }
            {
                std::lock_guard<std::mutex> lk(mtx);
                int sec_left = (int)std::chrono::duration_cast<std::chrono::seconds>(countdown_ms).count();
                if (sec_left < 0) sec_left = 0;
                std::string act;
                {
                    std::lock_guard<std::mutex> a_lk(action_mtx);
                    act = current_action;
                }
                draw_tui(all_repos, repo_infos, interval, sec_left, scanning, act, show_skipped);
            }
            std::this_thread::sleep_for(refresh_ms);
            countdown_ms -= refresh_ms;
        }

        running = false;
        if (scan_thread.joinable())
            scan_thread.join();
        if (logger_initialized())
            log_info("Program exiting");
    } catch (...) {
        throw;
    }
    return 0;
}
