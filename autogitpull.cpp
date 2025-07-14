#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <set>
#include <map>
#include <mutex>
#include <atomic>

#include "git_utils.h"
#include "repo_info.h"
#include "tui.h"
#include "arg_parser.h"

namespace fs = std::filesystem;

// Background thread: scan and update info
void scan_repos(
    const std::vector<fs::path>& all_repos,
    std::map<fs::path, git::RepoInfo>& repo_infos,
    std::set<fs::path>& skip_repos,
    std::mutex& mtx,
    std::atomic<bool>& scanning_flag,
    bool include_private)
{
    for (const auto& p : all_repos) {
        git::RepoInfo ri;
        ri.path = p;
        if (!fs::exists(p)) {
            ri.status = git::RS_ERROR;
            ri.message = "Missing";
            std::lock_guard<std::mutex> lk(mtx);
            repo_infos[p] = ri;
            continue;
        }
        if (skip_repos.count(p)) {
            ri.status = git::RS_SKIPPED;
            ri.message = "Skipped after fatal error";
            std::lock_guard<std::mutex> lk(mtx);
            repo_infos[p] = ri;
            continue;
        }
        try {
            ri.status = git::RS_CHECKING;
            ri.message = "";
            if (fs::is_directory(p) && git::is_git_repo(p)) {
                std::string origin = git::get_origin_url(p);
                if (!include_private) {
                    if (!git::is_github_url(origin)) {
                        ri.status = git::RS_SKIPPED;
                        ri.message = "Non-GitHub repo (skipped)";
                        std::lock_guard<std::mutex> lk(mtx);
                        repo_infos[p] = ri;
                        continue;
                    }
                    if (!git::remote_accessible(p)) {
                        ri.status = git::RS_SKIPPED;
                        ri.message = "Private or inaccessible repo";
                        std::lock_guard<std::mutex> lk(mtx);
                        repo_infos[p] = ri;
                        continue;
                    }
                }
                ri.branch = git::get_current_branch(p);
                if (ri.branch.empty() || ri.branch == "HEAD") {
                    ri.status = git::RS_HEAD_PROBLEM;
                    ri.message = "Detached HEAD or branch error";
                    skip_repos.insert(p);
                } else {
                    std::string local = git::get_local_hash(p);
                    std::string remote = git::get_remote_hash(p, ri.branch);
                    if (local.empty() || remote.empty()) {
                        ri.status = git::RS_ERROR;
                        ri.message = "Error getting hashes or remote";
                        skip_repos.insert(p);
                    } else if (local != remote) {
                        ri.status = git::RS_PULLING;
                        ri.message = "Remote ahead, pulling...";
                        {
                            std::lock_guard<std::mutex> lk(mtx);
                            repo_infos[p] = ri;
                        }
                        std::string pull_log;
                        int code = git::try_pull(p, pull_log);
                        ri.last_pull_log = pull_log;
                        if (code == 0) {
                            ri.status = git::RS_PULL_OK;
                            ri.message = "Pulled successfully";
                        } else if (code == 1) {
                            ri.status = git::RS_PKGLOCK_FIXED;
                            ri.message = "package-lock.json auto-reset & pulled";
                        } else {
                            ri.status = git::RS_ERROR;
                            ri.message = "Pull failed (see log)";
                            skip_repos.insert(p);
                        }
                    } else {
                        ri.status = git::RS_UP_TO_DATE;
                        ri.message = "Up to date";
                    }
                }
            } else {
                ri.status = git::RS_SKIPPED;
                ri.message = "Not a git repo (skipped)";
                skip_repos.insert(p);
            }
        } catch (const fs::filesystem_error& e) {
            ri.status = git::RS_ERROR;
            ri.message = e.what();
            skip_repos.insert(p);
        }
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
    }
    scanning_flag = false;
}

int main(int argc, char* argv[]) {
    tui::enable_win_ansi();
    ArgParser parser;
    ParsedArgs args = parser.parse(argc, argv);

    std::cout << "\033[?1049h"; // enter alternate buffer

    try {
        if (args.root.empty()) {
            std::cerr << "Usage: " << argv[0] << " <root-folder> [--include-private] [--show-skipped]\n";
            std::cout << "\033[?1049l" << std::flush;
            return 1;
        }
        fs::path root = args.root;
        if (!fs::exists(root) || !fs::is_directory(root)) {
            std::cerr << "Root path does not exist or is not a directory.\n";
            std::cout << "\033[?1049l" << std::flush;
            return 1;
        }

        std::vector<fs::path> all_repos;
        for (const auto& entry : fs::directory_iterator(root)) {
            all_repos.push_back(entry.path());
        }
        std::map<fs::path, git::RepoInfo> repo_infos;
        for (const auto& p : all_repos) {
            repo_infos[p] = git::RepoInfo{p, git::RS_CHECKING, "Pending..."};
        }

        const int interval = 60; // seconds
        std::set<fs::path> skip_repos;
        std::mutex mtx;
        std::atomic<bool> scanning(false);

        int countdown = 0; // Run immediately on start

        while (true) {
            if (countdown <= 0 && !scanning) {
                scanning = true;
                std::thread(scan_repos, std::cref(all_repos), std::ref(repo_infos),
                            std::ref(skip_repos), std::ref(mtx), std::ref(scanning),
                            args.include_private)
                    .detach();
                countdown = interval;
            }
            {
                std::lock_guard<std::mutex> lk(mtx);
                tui::draw_tui(all_repos, repo_infos, interval, countdown, scanning,
                              args.show_skipped);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            countdown--;
        }
    } catch (...) {
        std::cout << "\033[?1049l" << std::flush;
        throw;
    }
    std::cout << "\033[?1049l" << std::flush;
    return 0;
}
