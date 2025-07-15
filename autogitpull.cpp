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
#include <csignal>
#include "arg_parser.hpp"
#include "git_utils.hpp"
#include "tui.hpp"
#include "repo.hpp"

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
                  bool include_private,
                  const fs::path& log_dir,
                  bool check_only,
                  bool hash_check) {
    if (!running) return;
    {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = repo_infos.find(p);
        if (it != repo_infos.end() &&
            (it->second.status == RS_PULLING || it->second.status == RS_CHECKING)) {
            // Repo already being processed elsewhere
            std::cerr << "Skipping " << p << " - busy\n";
            return;
        }
    }
    RepoInfo ri;
    ri.path = p;
    ri.auth_failed = false;
    if (!fs::exists(p)) {
        ri.status = RS_ERROR;
        ri.message = "Missing";
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
        return;
    }
    if (skip_repos.count(p)) {
        ri.status = RS_SKIPPED;
        ri.message = "Skipped after fatal error";
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
        return;
    }
    try {
        ri.status = RS_CHECKING;
        ri.message = "";
        if (fs::is_directory(p) && git::is_git_repo(p)) {
            std::string origin = git::get_origin_url(p);
            if (!include_private) {
                if (!git::is_github_url(origin)) {
                    ri.status = RS_SKIPPED;
                    ri.message = "Non-GitHub repo (skipped)";
                    std::lock_guard<std::mutex> lk(mtx);
                    repo_infos[p] = ri;
                    return;
                }
                if (!git::remote_accessible(p)) {
                    ri.status = RS_SKIPPED;
                    ri.message = "Private or inaccessible repo";
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
                        needs_pull = false;
                    }
                }
                if (needs_pull) {
                    if (check_only) {
                        ri.status = RS_REMOTE_AHEAD;
                        ri.message = hash_check ? "Remote ahead" : "Update possible";
                    } else {
                        ri.status = RS_PULLING;
                        ri.message = "Remote ahead, pulling...";
                        ri.progress = 0;
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
                        } else if (code == 1) {
                            ri.status = RS_PKGLOCK_FIXED;
                            ri.message = "package-lock.json auto-reset & pulled";
                        } else {
                            ri.status = RS_ERROR;
                            ri.message = "Pull failed (see log)";
                            skip_repos.insert(p);
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
        }
    } catch (const fs::filesystem_error& e) {
        ri.status = RS_ERROR;
        ri.message = e.what();
        skip_repos.insert(p);
    }
    std::lock_guard<std::mutex> lk(mtx);
    repo_infos[p] = ri;
}

// Background thread: scan and update info
void scan_repos(
    const std::vector<fs::path>& all_repos,
    std::map<fs::path, RepoInfo>& repo_infos,
    std::set<fs::path>& skip_repos,
    std::mutex& mtx,
    std::atomic<bool>& scanning_flag,
    std::atomic<bool>& running,
    bool include_private,
    const fs::path& log_dir,
    bool check_only,
    bool hash_check,
    size_t concurrency
) {
    if (concurrency == 0) concurrency = 1;
    concurrency = std::min(concurrency, all_repos.size());

    std::atomic<size_t> next_index{0};
    auto worker = [&]() {
        while (running) {
            size_t idx = next_index.fetch_add(1);
            if (idx >= all_repos.size()) break;
            const auto& p = all_repos[idx];
            process_repo(p, repo_infos, skip_repos, mtx, running,
                         include_private, log_dir, check_only, hash_check);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(concurrency);
    for (size_t i = 0; i < concurrency; ++i)
        threads.emplace_back(worker);
    for (auto& t : threads) t.join();
    scanning_flag = false;
}

int main(int argc, char* argv[]) {
    git::GitInitGuard git_guard;
    try {
        const std::set<std::string> known{ "--include-private", "--show-skipped", "--interval", "--refresh-rate", "--log-dir", "--concurrency", "--check-only", "--no-hash-check", "--help" };
        ArgParser parser(argc, argv, known);

        if (parser.has_flag("--help")) {
            std::cout << "Usage: " << argv[0]
                      << " <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--refresh-rate <ms>] [--log-dir <path>] [--concurrency <n>] [--check-only] [--no-hash-check] [--help]\n";
            return 0;
        }

        if (parser.positional().size() != 1) {
            std::cerr << "Usage: " << argv[0]
                      << " <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--refresh-rate <ms>] [--log-dir <path>] [--concurrency <n>] [--check-only] [--no-hash-check] [--help]\n";
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
        size_t concurrency = 3;
        int interval = 30;
        std::chrono::milliseconds refresh_ms(500);
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
        fs::path root = parser.positional().front();
        if (!fs::exists(root) || !fs::is_directory(root)) {
            std::cerr << "Root path does not exist or is not a directory.\n";
            return 1;
        }

        // Grab all first-level subdirs at startup (fixed list)
        std::vector<fs::path> all_repos;
        for (const auto& entry : fs::directory_iterator(root)) {
            all_repos.push_back(entry.path());
        }
        std::map<fs::path, RepoInfo> repo_infos;
        for (const auto& p : all_repos) {
            repo_infos[p] = RepoInfo{p, RS_CHECKING, "Pending...", "", "", 0, false};
        }

        std::set<fs::path> skip_repos;
        std::mutex mtx;
        std::atomic<bool> scanning(false);
        std::atomic<bool> running(true);

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
                scanning = true;
                scan_thread = std::thread(scan_repos, std::cref(all_repos), std::ref(repo_infos),
                                          std::ref(skip_repos), std::ref(mtx),
                                          std::ref(scanning), std::ref(running), include_private,
                                          std::cref(log_dir), check_only, hash_check, concurrency);
                countdown_ms = std::chrono::seconds(interval);
            }
            {
                std::lock_guard<std::mutex> lk(mtx);
                int sec_left = (int)std::chrono::duration_cast<std::chrono::seconds>(countdown_ms).count();
                if (sec_left < 0) sec_left = 0;
                draw_tui(all_repos, repo_infos, interval, sec_left, scanning, show_skipped);
            }
            std::this_thread::sleep_for(refresh_ms);
            countdown_ms -= refresh_ms;
        }

        running = false;
        if (scan_thread.joinable())
            scan_thread.join();
    } catch (...) {
        throw;
    }
    return 0;
}
