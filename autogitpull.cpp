#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <set>
#include <map>
#include <mutex>
#include <atomic>
#include <sstream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
const char* QUOTE = "\"";
#else
const char* QUOTE = "'";
#endif

namespace fs = std::filesystem;

// ANSI color codes
const char* COLOR_RESET = "\033[0m";
const char* COLOR_GREEN = "\033[32m";
const char* COLOR_YELLOW = "\033[33m";
const char* COLOR_RED = "\033[31m";
const char* COLOR_CYAN = "\033[36m";
const char* COLOR_GRAY = "\033[90m";
const char* COLOR_BOLD = "\033[1m";

#ifdef _WIN32
void enable_win_ansi() {
    // Enable ANSI color on Windows 10+ console
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    SetConsoleMode(hOut, dwMode);
}
#else
void enable_win_ansi() {} // No-op on non-Windows
#endif


std::string timestamp() {
    std::time_t now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

struct PopenHandle {
    FILE* fp;
    ~PopenHandle() {
        if (fp)
            pclose(fp);
    }
};

std::string run_cmd(const std::string& cmd, int timeout_sec = 0) {
#ifndef _WIN32
    std::string final_cmd = cmd;
    if (timeout_sec > 0) {
        final_cmd = "timeout " + std::to_string(timeout_sec) + " " + cmd;
    }
#else
    std::string final_cmd = cmd; // Timeout not supported on Windows fallback
    (void)timeout_sec;
#endif

    std::string data;
    char buffer[256];
    FILE* raw = popen(final_cmd.c_str(), "r");
    PopenHandle stream{raw};
    if (stream.fp) {
        while (fgets(buffer, sizeof(buffer), stream.fp)) {
            data.append(buffer);
        }
    }
    if (!data.empty() && data.back() == '\n')
        data.pop_back();
    return data;
}

// Quote a filesystem path for safe command usage
std::string quote_path(const fs::path& p) {
    return std::string(QUOTE) + p.string() + std::string(QUOTE);
}

bool is_git_repo(const fs::path& p) {
    return fs::exists(p / ".git") && fs::is_directory(p / ".git");
}

std::string get_local_hash(const fs::path& repo) {
    std::string cmd = "cd " + quote_path(repo)
        + " && git rev-parse HEAD 2>&1";
    return run_cmd(cmd, 30);
}

std::string get_current_branch(const fs::path& repo) {
    std::string cmd = "cd " + quote_path(repo)
        + " && git rev-parse --abbrev-ref HEAD 2>&1";
    return run_cmd(cmd, 30);
}

std::string get_remote_hash(const fs::path& repo, const std::string& branch) {
    std::string fetch_cmd = "cd " + quote_path(repo)
        + " && git fetch --quiet 2>&1";
    run_cmd(fetch_cmd, 30);
    std::string cmd = "cd " + quote_path(repo)
        + " && git rev-parse origin/" + branch + " 2>&1";
    return run_cmd(cmd, 30);
}

std::string get_origin_url(const fs::path& repo) {
    std::string cmd = "cd " + quote_path(repo)
        + " && git config --get remote.origin.url 2>&1";
    return run_cmd(cmd, 15);
}

bool is_github_url(const std::string& url) {
    return url.find("github.com") != std::string::npos;
}

bool remote_accessible(const fs::path& repo) {
    std::string cmd = "cd " + quote_path(repo)
        + " && git ls-remote -h origin HEAD 2>&1";
    std::string out = run_cmd(cmd, 30);
    if (out.find("fatal") != std::string::npos || out.find("Permission denied") != std::string::npos
        || out.find("ERROR") != std::string::npos)
        return false;
    return true;
}

// Return codes: 0 = ok, 1 = package-lock reset+pull, 2 = error
int try_pull(const fs::path& repo, std::string& out_pull_log) {
    std::string pull = run_cmd("cd " + quote_path(repo) + " && git pull 2>&1", 30);
    out_pull_log = pull;
    if (pull.find("package-lock.json") != std::string::npos && pull.find("Please commit your changes or stash them before you merge") != std::string::npos) {
        run_cmd("cd " + quote_path(repo) + " && git checkout -- package-lock.json", 30);
        pull = run_cmd("cd " + quote_path(repo) + " && git pull 2>&1", 30);
        out_pull_log += "\n[Auto-discarded package-lock.json]\n" + pull;
        if (pull.find("Already up to date") != std::string::npos || pull.find("Updating") != std::string::npos)
            return 1;
        else
            return 2;
    }
    if (pull.find("Already up to date") != std::string::npos || pull.find("Updating") != std::string::npos)
        return 0;
    return 2;
}

enum RepoStatus {
    RS_CHECKING,
    RS_UP_TO_DATE,
    RS_PULLING,
    RS_PULL_OK,
    RS_PKGLOCK_FIXED,
    RS_ERROR,
    RS_SKIPPED,
    RS_HEAD_PROBLEM
};

struct RepoInfo {
    fs::path path;
    RepoStatus status = RS_CHECKING;
    std::string message;
    std::string branch;
    std::string last_pull_log;
};

void draw_tui(const std::vector<fs::path>& all_repos, const std::map<fs::path, RepoInfo>& repo_infos,
              int interval, int seconds_left, bool scanning, bool show_skipped) {
    std::ostringstream out;
    out << "\033[2J\033[H";
    out << COLOR_BOLD << "AutoGitPull TUI   " << COLOR_RESET
        << COLOR_CYAN << timestamp() << COLOR_RESET
        << "   Monitoring: " << COLOR_YELLOW << (all_repos.empty() ? "" : all_repos[0].parent_path().string()) << COLOR_RESET << "\n";
    out << "Interval: " << interval << "s    (Ctrl+C to exit)\n";
    if (scanning) out << COLOR_YELLOW << "Scan in progress...\n" << COLOR_RESET;
    out << "---------------------------------------------------------------------\n";
    out << COLOR_BOLD << "  Status     Repo" << COLOR_RESET << "\n";
    out << "---------------------------------------------------------------------\n";
    for (const auto& p : all_repos) {
        RepoInfo ri;
        auto it = repo_infos.find(p);
        if (it != repo_infos.end())
            ri = it->second;
        else {
            ri.path = p;
            ri.status = RS_CHECKING;
            ri.message = "Pending...";
        }
        if (ri.status == RS_SKIPPED && !show_skipped)
            continue;
        std::string color = COLOR_GRAY, status_s = "CHECK    ";
        switch (ri.status) {
            case RS_CHECKING:      color = COLOR_CYAN;   status_s = "Checking "; break;
            case RS_UP_TO_DATE:    color = COLOR_GREEN;  status_s = "UpToDate "; break;
            case RS_PULLING:       color = COLOR_YELLOW; status_s = "Pulling  "; break;
            case RS_PULL_OK:       color = COLOR_GREEN;  status_s = "Pulled   "; break;
            case RS_PKGLOCK_FIXED: color = COLOR_YELLOW; status_s = "PkgLockOk"; break;
            case RS_ERROR:         color = COLOR_RED;    status_s = "Error    "; break;
            case RS_SKIPPED:       color = COLOR_GRAY;   status_s = "Skipped  "; break;
            case RS_HEAD_PROBLEM:  color = COLOR_RED;    status_s = "HEAD/BR  "; break;
        }
        out << color << " [" << std::left << std::setw(9) << status_s << "]  " << p.filename().string() << COLOR_RESET;
        if (!ri.branch.empty()) out << "  (" << ri.branch << ")";
        if (!ri.message.empty()) out << " - " << ri.message;
        out << "\n";
    }
    out << "---------------------------------------------------------------------\n";
    out << COLOR_CYAN << "Next scan in " << seconds_left << " seconds..." << COLOR_RESET;
    std::cout << out.str() << std::flush;
}

// Background thread: scan and update info
void scan_repos(
    const std::vector<fs::path>& all_repos,
    std::map<fs::path, RepoInfo>& repo_infos,
    std::set<fs::path>& skip_repos,
    std::mutex& mtx,
    std::atomic<bool>& scanning_flag,
    bool include_private
) {
    for (const auto& p : all_repos) {
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
            if (fs::is_directory(p) && is_git_repo(p)) {
                std::string origin = get_origin_url(p);
                if (!include_private) {
                    if (!is_github_url(origin)) {
                        ri.status = RS_SKIPPED;
                        ri.message = "Non-GitHub repo (skipped)";
                        std::lock_guard<std::mutex> lk(mtx);
                        repo_infos[p] = ri;
                        continue;
                    }
                    if (!remote_accessible(p)) {
                        ri.status = RS_SKIPPED;
                        ri.message = "Private or inaccessible repo";
                        std::lock_guard<std::mutex> lk(mtx);
                        repo_infos[p] = ri;
                        continue;
                    }
                }
                ri.branch = get_current_branch(p);
                if (ri.branch.empty() || ri.branch == "HEAD") {
                    ri.status = RS_HEAD_PROBLEM;
                    ri.message = "Detached HEAD or branch error";
                    skip_repos.insert(p);
                } else {
                    std::string local = get_local_hash(p);
                    std::string remote = get_remote_hash(p, ri.branch);
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
                        int code = try_pull(p, pull_log);
                        ri.last_pull_log = pull_log;
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
    enable_win_ansi();
    std::cout << "\033[?1049h"; // enter alternate buffer
    try {
        if (argc < 2 || argc > 4) {
            std::cerr << "Usage: " << argv[0] << " <root-folder> [--include-private] [--show-skipped]\n";
            std::cout << "\033[?1049l" << std::flush; // leave alt buffer
            return 1;
        }
        bool include_private = false;
        bool show_skipped = false;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--include-private")
                include_private = true;
            else if (arg == "--show-skipped")
                show_skipped = true;
            else {
                std::cerr << "Unknown option: " << arg << "\n";
                std::cout << "\033[?1049l" << std::flush;
                return 1;
            }
        }
        fs::path root = argv[1];
        if (!fs::exists(root) || !fs::is_directory(root)) {
            std::cerr << "Root path does not exist or is not a directory.\n";
            std::cout << "\033[?1049l" << std::flush; // leave alt buffer
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

        const int interval = 60; // seconds
        std::set<fs::path> skip_repos;
        std::mutex mtx;
        std::atomic<bool> scanning(false);

        int countdown = 0; // Run immediately on start

        while (true) {
            if (countdown <= 0 && !scanning) {
                scanning = true;
                std::thread(scan_repos, std::cref(all_repos), std::ref(repo_infos), std::ref(skip_repos), std::ref(mtx), std::ref(scanning), include_private).detach();
                countdown = interval;
            }
            {
                std::lock_guard<std::mutex> lk(mtx);
                draw_tui(all_repos, repo_infos, interval, countdown, scanning, show_skipped);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            countdown--;
        }
    } catch (...) {
        std::cout << "\033[?1049l" << std::flush; // leave alt buffer on any error
        throw;
    }
    std::cout << "\033[?1049l" << std::flush; // leave alt buffer on normal exit
    return 0;
}
