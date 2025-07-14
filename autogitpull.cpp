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


// Background thread: scan and update info
void scan_repos(
    const std::vector<fs::path>& all_repos,
    std::map<fs::path, RepoInfo>& repo_infos,
    std::set<fs::path>& skip_repos,
    std::mutex& mtx,
    std::atomic<bool>& scanning_flag,
    std::atomic<bool>& running,
    bool include_private,
    const fs::path& log_dir
) {
    for (const auto& p : all_repos) {
        if (!running) break;
        RepoInfo ri;
        ri.path = p;
        if (!fs::exists(p)) {
            ri.status = RS_ERROR;
            ri.message = "Missing";
            std::lock_guard<std::mutex> lk(mtx);
            repo_infos[p] = ri;
            continue;
        }
        if (skip_repos.count(p)) {
            ri.status = RS_SKIPPED;
            ri.message = "Skipped after fatal error";
            std::lock_guard<std::mutex> lk(mtx);
            repo_infos[p] = ri;
            continue;
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
                        continue;
                    }
                    if (!git::remote_accessible(p)) {
                        ri.status = RS_SKIPPED;
                        ri.message = "Private or inaccessible repo";
                        std::lock_guard<std::mutex> lk(mtx);
                        repo_infos[p] = ri;
                        continue;
                    }
                }
                ri.branch = git::get_current_branch(p);
                if (ri.branch.empty() || ri.branch == "HEAD") {
                    ri.status = RS_HEAD_PROBLEM;
                    ri.message = "Detached HEAD or branch error";
                    skip_repos.insert(p);
                } else {
                    std::string local = git::get_local_hash(p);
                    std::string remote = git::get_remote_hash(p, ri.branch);
                    if (local.empty() || remote.empty()) {
                        ri.status = RS_ERROR;
                        ri.message = "Error getting hashes or remote";
                        skip_repos.insert(p);
                    } else if (local != remote) {
                        ri.status = RS_PULLING;
                        ri.message = "Remote ahead, pulling...";
                        {
                            std::lock_guard<std::mutex> lk(mtx);
                            repo_infos[p] = ri;
                        }
                        std::string pull_log;
                        int code = git::try_pull(p, pull_log);
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
                    } else {
                        ri.status = RS_UP_TO_DATE;
                        ri.message = "Up to date";
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
    scanning_flag = false;
}

int main(int argc, char* argv[]) {
    AltScreenGuard guard;
    git::GitInitGuard git_guard;
    try {
        const std::set<std::string> known{ "--include-private", "--show-skipped", "--interval", "--log-dir", "--help" };
        ArgParser parser(argc, argv, known);

        if (parser.has_flag("--help")) {
            std::cout << "Usage: " << argv[0]
                      << " <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--log-dir <path>] [--help]\n";
            return 0;
        }

        if (parser.positional().size() != 1) {
            std::cerr << "Usage: " << argv[0]
                      << " <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--log-dir <path>] [--help]\n";
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
        int interval = 60;
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
            repo_infos[p] = RepoInfo{p, RS_CHECKING, "Pending..."};
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
        int countdown = 0; // Run immediately on start

        while (running) {
            if (!scanning && scan_thread.joinable())
                scan_thread.join();

            if (running && countdown <= 0 && !scanning) {
                scanning = true;
                scan_thread = std::thread(scan_repos, std::cref(all_repos), std::ref(repo_infos),
                                          std::ref(skip_repos), std::ref(mtx),
                                          std::ref(scanning), std::ref(running), include_private,
                                          std::cref(log_dir));
                countdown = interval;
            }
            {
                std::lock_guard<std::mutex> lk(mtx);
                draw_tui(all_repos, repo_infos, interval, countdown, scanning, show_skipped);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            countdown--;
        }

        running = false;
        if (scan_thread.joinable())
            scan_thread.join();
    } catch (...) {
        throw;
    }
    return 0;
}
