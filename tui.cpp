#include "tui.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>
#include <filesystem>
#include <string>
#include "git_utils.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

const char* COLOR_RESET = "\033[0m";
const char* COLOR_GREEN = "\033[32m";
const char* COLOR_YELLOW = "\033[33m";
const char* COLOR_RED = "\033[31m";
const char* COLOR_CYAN = "\033[36m";
const char* COLOR_GRAY = "\033[90m";
const char* COLOR_BOLD = "\033[1m";
const char* COLOR_MAGENTA = "\033[35m";

#ifdef _WIN32
void enable_win_ansi() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    SetConsoleMode(hOut, dwMode);
}
#else
void enable_win_ansi() {}
#endif

std::string timestamp() {
    std::time_t now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

void draw_tui(const std::vector<fs::path>& all_repos,
              const std::map<fs::path, RepoInfo>& repo_infos,
              int interval, int seconds_left, bool scanning, bool show_skipped) {
    std::ostringstream out;
    out << "\033[2J\033[H";
    out << COLOR_BOLD << "AutoGitPull TUI   " << COLOR_RESET
        << COLOR_CYAN << timestamp() << COLOR_RESET << "\n";
    out << "Monitoring: "
        << COLOR_YELLOW
        << (all_repos.empty() ? "" : all_repos[0].parent_path().string())
        << COLOR_RESET << "\n";
    out << "Interval: " << interval << "s    (Ctrl+C to exit)\n";
    if (scanning) out << COLOR_YELLOW << "Scan in progress...\n" << COLOR_RESET;
    out << "--------------------------------------------------------------";
    out << "-------------------\n";
    out << COLOR_BOLD << "  Status     Repo" << COLOR_RESET << "\n";
    out << "--------------------------------------------------------------";
    out << "-------------------\n";
    for (const auto& p : all_repos) {
        RepoInfo ri;
        auto it = repo_infos.find(p);
        if (it != repo_infos.end())
            ri = it->second;
        else {
            ri.status = RS_CHECKING;
            ri.message = "Pending...";
            ri.path = p;
            ri.auth_failed = false;
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
            case RS_REMOTE_AHEAD:  color = COLOR_MAGENTA;status_s = "RemoteUp"; break;
        }
        out << color << " [" << std::left << std::setw(9) << status_s << "]  "
            << p.filename().string() << COLOR_RESET;
        if (!ri.branch.empty()) out << "  (" << ri.branch << ")";
        if (!ri.message.empty()) out << " - " << ri.message;
        if (ri.auth_failed) out << COLOR_RED << " [AUTH]" << COLOR_RESET;
        if (ri.status == RS_PULLING)
            out << " (" << ri.progress << "%)";
        out << "\n";
    }
    out << "--------------------------------------------------------------";
    out << "-------------------\n";
    out << COLOR_CYAN << "Next scan in " << seconds_left << " seconds..." << COLOR_RESET;
    std::cout << out.str() << std::flush;
}
