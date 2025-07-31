#include "tui.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <string>
#include "time_utils.hpp"
#include "git_utils.hpp"
#include "resource_utils.hpp"
#include "system_utils.hpp"
#include "version.hpp"

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
    if (hOut == INVALID_HANDLE_VALUE)
        return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return;
    dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    SetConsoleMode(hOut, dwMode);
}
#else
void enable_win_ansi() {}
#endif

void draw_tui(const std::vector<fs::path>& all_repos,
              const std::map<fs::path, RepoInfo>& repo_infos, int interval, int seconds_left,
              bool scanning, const std::string& action, bool show_skipped, bool show_version,
              bool track_cpu, bool track_mem, bool track_threads, bool track_net,
              bool show_affinity, bool track_vmem, bool show_commit_date, bool show_commit_author,
              bool session_dates_only, bool no_colors, const std::string& custom_color,
              const std::string& status_msg, int runtime_sec, bool show_datetime_line,
              bool show_header, bool show_repo_count) {
    std::ostringstream out;
    auto choose = [&](const char* def) {
        return no_colors ? "" : (custom_color.empty() ? def : custom_color.c_str());
    };
    std::string reset = no_colors ? "" : COLOR_RESET;
    std::string green = choose(COLOR_GREEN);
    std::string yellow = choose(COLOR_YELLOW);
    std::string red = choose(COLOR_RED);
    std::string cyan = choose(COLOR_CYAN);
    std::string gray = choose(COLOR_GRAY);
    std::string bold = choose(COLOR_BOLD);
    std::string magenta = choose(COLOR_MAGENTA);
    out << "\033[2J\033[H";
    out << bold << "AutoGitPull TUI";
    if (show_version)
        out << " v" << AUTOGITPULL_VERSION;
    out << reset << "\n";
    if (show_datetime_line)
        out << "Date: " << cyan << timestamp() << reset << "\n";
    out << "Monitoring: " << yellow
        << (all_repos.empty() ? "" : all_repos[0].parent_path().string()) << reset << "\n";
    if (show_repo_count) {
        size_t active = 0;
        for (const auto& p : all_repos) {
            auto it = repo_infos.find(p);
            RepoStatus st = it != repo_infos.end() ? it->second.status : RS_PENDING;
            if (st != RS_SKIPPED)
                ++active;
        }
        out << "Repos: " << active << "/" << all_repos.size() << "\n";
    }
    out << "Interval: " << interval << "s    (Ctrl+C to exit)\n";
    out << "Status: ";
    if (scanning || action != "Idle")
        out << COLOR_YELLOW << action << COLOR_RESET;
    else
        out << green << "Idle" << reset;
    out << " - Next scan in " << seconds_left << "s";
    if (runtime_sec >= 0)
        out << " - Runtime " << format_duration_short(std::chrono::seconds(runtime_sec));
    out << "\n";
    if (!status_msg.empty())
        out << status_msg << "\n";
    if (track_cpu || track_mem || track_threads || show_affinity || track_vmem) {
        out << "CPU: ";
        if (track_cpu)
            out << std::fixed << std::setprecision(1) << procutil::get_cpu_percent() << "% ";
        else
            out << "N/A ";
        out << "  Mem: ";
        if (track_mem)
            out << procutil::get_memory_usage_mb() << " MB";
        else
            out << "N/A";
        if (track_vmem)
            out << "  VMem: " << procutil::get_virtual_memory_kb() / 1024 << " MB";
        out << "  Threads: ";
        if (track_threads)
            out << procutil::get_thread_count();
        else
            out << "N/A";
        if (show_affinity) {
            std::string mask = procutil::get_cpu_affinity();
            if (!mask.empty())
                out << "  Core: " << mask;
        }
        out << "\n";
    }
    if (track_net) {
        auto usage = procutil::get_network_usage();
        auto fmt = [](std::size_t b) {
            std::ostringstream ss;
            if (b >= 1024 * 1024)
                ss << b / (1024 * 1024) << " MB";
            else
                ss << b / 1024 << " KB";
            return ss.str();
        };
        out << "Net: D " << fmt(usage.download_bytes) << "  U " << fmt(usage.upload_bytes) << "\n";
    }
    if (show_header) {
        out << "--------------------------------------------------------------";
        out << "-------------------\n";
        out << bold << " [" << std::left << std::setw(9) << "Status" << "]  Repo" << reset << "\n";
        out << "--------------------------------------------------------------";
        out << "-------------------\n";
    }
    for (const auto& p : all_repos) {
        RepoInfo ri;
        auto it = repo_infos.find(p);
        if (it != repo_infos.end())
            ri = it->second;
        else {
            ri.status = RS_PENDING;
            ri.message = "Pending...";
            ri.path = p;
            ri.auth_failed = false;
        }
        if (ri.status == RS_SKIPPED && !show_skipped)
            continue;
        std::string color = gray, status_s = "Pending ";
        switch (ri.status) {
        case RS_PENDING:
            color = COLOR_GRAY;
            status_s = "Pending ";
            break;
        case RS_CHECKING:
            color = cyan;
            status_s = "Checking ";
            break;
        case RS_UP_TO_DATE:
            color = green;
            status_s = "UpToDate ";
            break;
        case RS_PULLING:
            color = yellow;
            status_s = "Pulling  ";
            break;
        case RS_PULL_OK:
            color = green;
            status_s = "Pulled   ";
            break;
        case RS_PKGLOCK_FIXED:
            color = yellow;
            status_s = "PkgLockOk";
            break;
        case RS_ERROR:
            color = red;
            status_s = "Error    ";
            break;
        case RS_SKIPPED:
            color = gray;
            status_s = "Skipped  ";
            break;
        case RS_HEAD_PROBLEM:
            color = red;
            status_s = "HEAD/BR  ";
            break;
        case RS_DIRTY:
            color = red;
            status_s = "Dirty    ";
            break;
        case RS_REMOTE_AHEAD:
            color = magenta;
            status_s = "RemoteUp";
            break;
        }
        out << color << " [" << std::left << std::setw(9) << status_s << "]  "
            << p.filename().string() << reset;
        if (!ri.branch.empty()) {
            out << "  (" << ri.branch;
            if (!ri.commit.empty())
                out << "@" << ri.commit;
            out << ")";
        }
        if ((!session_dates_only || ri.pulled) && (show_commit_author || show_commit_date)) {
            out << " {";
            bool first = true;
            if (show_commit_author && !ri.commit_author.empty()) {
                out << ri.commit_author;
                first = false;
            }
            if (show_commit_date && !ri.commit_date.empty()) {
                if (!first)
                    out << ' ';
                out << ri.commit_date;
            }
            out << "}";
        }
        if (!ri.message.empty())
            out << " - " << ri.message;
        if (ri.auth_failed)
            out << red << " [AUTH]" << reset;
        if (ri.status == RS_PULLING)
            out << " (" << ri.progress << "%)";
        out << "\n";
    }
    if (show_header) {
        out << "--------------------------------------------------------------";
        out << "-------------------\n";
    }
    std::cout << out.str() << std::flush;
    // Explicitly clear the string buffer to avoid stale allocations
    out.str("");
    out.clear();
    std::ostringstream().swap(out);
}
