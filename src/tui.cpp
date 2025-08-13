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

static std::string format_bytes(std::size_t b) {
    std::ostringstream ss;
    if (b >= 1024 * 1024)
        ss << b / (1024 * 1024) << " MB";
    else
        ss << b / 1024 << " KB";
    return ss.str();
}

TuiColors make_tui_colors(bool no_colors, const std::string& custom_color) {
    auto choose = [&](const char* def) {
        return no_colors ? "" : (custom_color.empty() ? def : custom_color.c_str());
    };
    return {no_colors ? "" : COLOR_RESET, choose(COLOR_GREEN),
            choose(COLOR_YELLOW),         choose(COLOR_RED),
            choose(COLOR_CYAN),           choose(COLOR_GRAY),
            choose(COLOR_BOLD),           choose(COLOR_MAGENTA)};
}

std::string render_header(const std::vector<fs::path>& all_repos,
                          const std::map<fs::path, RepoInfo>& repo_infos, int interval,
                          int seconds_left, bool scanning, const std::string& action,
                          bool show_version, bool show_repo_count, const std::string& status_msg,
                          int runtime_sec, bool show_datetime_line, const TuiColors& c) {
    std::ostringstream out;
    out << "\033[2J\033[H";
    out << c.bold << "AutoGitPull TUI";
    if (show_version)
        out << " v" << AUTOGITPULL_VERSION;
    out << c.reset << "\n";
    if (show_datetime_line)
        out << "Date: " << c.cyan << timestamp() << c.reset << "\n";
    out << "Monitoring: " << c.yellow
        << (all_repos.empty() ? "" : all_repos[0].parent_path().string()) << c.reset << "\n";
    if (show_repo_count) {
        size_t active = 0;
        for (const auto& p : all_repos) {
            auto it = repo_infos.find(p);
            RepoStatus st = it != repo_infos.end() ? it->second.status : RS_PENDING;
            if (st != RS_SKIPPED && st != RS_NOT_GIT)
                ++active;
        }
        out << "Repos: " << active << "/" << all_repos.size() << "\n";
    }
    out << "Interval: " << interval << "s    (Ctrl+C to exit)\n";
    out << "Status: ";
    if (scanning || action != "Idle")
        out << c.yellow << action << c.reset;
    else
        out << c.green << "Idle" << c.reset;
    out << " - Next scan in " << seconds_left << "s";
    if (runtime_sec >= 0)
        out << " - Runtime " << format_duration_short(std::chrono::seconds(runtime_sec));
    out << "\n";
    if (!status_msg.empty())
        out << status_msg << "\n";
    return out.str();
}

std::string render_stats(bool track_cpu, bool track_mem, bool track_threads, bool track_net,
                         bool show_affinity, bool track_vmem, const TuiColors& c) {
    std::ostringstream out;
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
        out << "Net: D " << format_bytes(usage.download_bytes) << "  U "
            << format_bytes(usage.upload_bytes) << "\n";
    }
    return out.str();
}

std::string render_repo_entry(const fs::path& p, const RepoInfo& ri, bool show_skipped,
                              bool show_notgit, bool show_commit_date, bool show_commit_author,
                              bool session_dates_only, bool censor_names, char censor_char,
                              const TuiColors& c) {
    if ((ri.status == RS_SKIPPED && !show_skipped) || (ri.status == RS_NOT_GIT && !show_notgit))
        return "";
    std::string color = c.gray, status_s = "Pending ";
    switch (ri.status) {
    case RS_PENDING:
        color = c.gray;
        status_s = "Pending ";
        break;
    case RS_CHECKING:
        color = c.cyan;
        status_s = "Checking ";
        break;
    case RS_UP_TO_DATE:
        color = c.green;
        status_s = "UpToDate ";
        break;
    case RS_PULLING:
        color = c.yellow;
        status_s = "Pulling  ";
        break;
    case RS_PULL_OK:
        color = c.green;
        status_s = "Pulled   ";
        break;
    case RS_PKGLOCK_FIXED:
        color = c.yellow;
        status_s = "PkgLockOk";
        break;
    case RS_ERROR:
        color = c.red;
        status_s = "Error    ";
        break;
    case RS_SKIPPED:
        color = c.gray;
        status_s = "Skipped  ";
        break;
    case RS_NOT_GIT:
        color = c.gray;
        status_s = "NotGit  ";
        break;
    case RS_HEAD_PROBLEM:
        color = c.red;
        status_s = "HEAD/BR  ";
        break;
    case RS_DIRTY:
        color = c.red;
        status_s = "Dirty    ";
        break;
    case RS_TIMEOUT:
        color = c.red;
        status_s = "TimedOut ";
        break;
    case RS_RATE_LIMIT:
        color = c.red;
        status_s = "RateLimit";
        break;
    case RS_REMOTE_AHEAD:
        color = c.magenta;
        status_s = "RemoteUp";
        break;
    case RS_TEMPFAIL:
        color = c.red;
        status_s = "TempFail";
        break;
    }
    std::ostringstream out;
    std::string name = p.filename().string();
    if (censor_names)
        name.assign(name.size(), censor_char);
    out << color << " [" << std::left << std::setw(9) << status_s << "]  " << name << c.reset;
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
        out << c.red << " [AUTH]" << c.reset;
    if (ri.status == RS_PULLING)
        out << " (" << ri.progress << "%)";
    out << "\n";
    return out.str();
}

void draw_tui(const std::vector<fs::path>& all_repos,
              const std::map<fs::path, RepoInfo>& repo_infos, int interval, int seconds_left,
              bool scanning, const std::string& action, bool show_skipped, bool show_notgit,
              bool show_version, bool track_cpu, bool track_mem, bool track_threads, bool track_net,
              bool show_affinity, bool track_vmem, bool show_commit_date, bool show_commit_author,
              bool session_dates_only, bool no_colors, const std::string& custom_color,
              const std::string& status_msg, int runtime_sec, bool show_datetime_line,
              bool show_header, bool show_repo_count, bool censor_names, char censor_char) {
    TuiColors colors = make_tui_colors(no_colors, custom_color);
    std::ostringstream out;
    out << render_header(all_repos, repo_infos, interval, seconds_left, scanning, action,
                         show_version, show_repo_count, status_msg, runtime_sec, show_datetime_line,
                         colors);
    out << render_stats(track_cpu, track_mem, track_threads, track_net, show_affinity, track_vmem,
                        colors);
    if (show_header) {
        out << "--------------------------------------------------------------";
        out << "-------------------\n";
        out << colors.bold << " [" << std::left << std::setw(9) << "Status" << "]  Repo"
            << colors.reset << "\n";
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
        out << render_repo_entry(p, ri, show_skipped, show_notgit, show_commit_date,
                                 show_commit_author, session_dates_only, censor_names, censor_char,
                                 colors);
    }
    if (show_header) {
        out << "--------------------------------------------------------------";
        out << "-------------------\n";
    }
    std::cout << out.str() << std::flush;
    out.str("");
    out.clear();
    std::ostringstream().swap(out);
}
