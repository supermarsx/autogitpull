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

#ifdef _WIN32
/**
 * @brief Enable ANSI escape sequence processing on Windows consoles.
 *
 * Adjusts the console mode so that emitted ANSI color codes are interpreted
 * correctly. The function takes no parameters and returns no value.
 */
void enable_win_ansi() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return;
    // Enable processing of ANSI escape sequences
    dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    SetConsoleMode(hOut, dwMode);
}
#else
/**
 * @brief Stub for non-Windows platforms where ANSI sequences already work.
 */
void enable_win_ansi() {}
#endif

/**
 * @brief Convert a byte count into a human-readable string.
 *
 * Values ≥1 MB are formatted in megabytes, otherwise in kilobytes.
 *
 * @param b Number of bytes to format.
 * @return Size string such as "10 MB" or "512 KB".
 */
static std::string format_bytes(std::size_t b) {
    std::ostringstream ss;
    if (b >= 1024 * 1024)
        ss << b / (1024 * 1024) << " MB";
    else
        ss << b / 1024 << " KB";
    return ss.str();
}

/**
 * @brief Build the ANSI color palette used by the TUI.
 *
 * @param no_colors When true, all color codes are suppressed.
 * @param custom_color ANSI escape sequence that overrides theme defaults if
 *                     not empty.
 * @param theme Default set of color codes.
 * @return Struct containing the escape sequences to use for each color.
 */
TuiColors make_tui_colors(bool no_colors, const std::string& custom_color, const TuiTheme& theme) {
    auto choose = [&](const std::string& def) {
        // If colors are disabled, return an empty string; otherwise prefer a
        // custom color when provided and fall back to the theme default.
        return no_colors ? std::string() : (custom_color.empty() ? def : custom_color);
    };
    return {no_colors ? std::string() : theme.reset,
            choose(theme.green),
            choose(theme.yellow),
            choose(theme.red),
            choose(theme.cyan),
            choose(theme.gray),
            choose(theme.bold),
            choose(theme.magenta)};
}

/**
 * @brief Render the top header section of the TUI.
 *
 * @param all_repos       List of repositories being monitored.
 * @param repo_infos      Map of repository information keyed by path.
 * @param interval        Polling interval in seconds.
 * @param seconds_left    Seconds remaining until the next scan.
 * @param scanning        Whether a scan is currently in progress.
 * @param action          Text describing the current action.
 * @param show_version    Display program version when true.
 * @param show_repo_count Include repository counts in output.
 * @param status_msg      Additional status message to display.
 * @param runtime_sec     Application runtime in seconds, or -1 to hide.
 * @param show_datetime_line Show current date/time line when true.
 * @param c               ANSI color palette.
 * @return Formatted header string ready for terminal output.
 */
std::string render_header(const std::vector<fs::path>& all_repos,
                          const std::map<fs::path, RepoInfo>& repo_infos, int interval,
                          int seconds_left, bool scanning, const std::string& action,
                          bool show_version, bool show_repo_count, const std::string& status_msg,
                          int runtime_sec, bool show_datetime_line, const TuiColors& c) {
    std::ostringstream out;
    out << "\033[2J\033[H";             // Clear screen and move cursor to home
    out << c.bold << "AutoGitPull TUI"; // Program title in bold
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
        out << c.yellow << action << c.reset; // Highlight active operations
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

/**
 * @brief Render process resource usage statistics.
 *
 * @param track_cpu      Include CPU percentage when true.
 * @param track_mem      Include resident memory usage.
 * @param track_threads  Include thread count.
 * @param track_net      Include network I/O statistics.
 * @param show_affinity  Include CPU affinity mask.
 * @param track_vmem     Include virtual memory usage.
 * @param c              ANSI color palette (unused but reserved for future styling).
 * @return Formatted statistics string.
 */
std::string render_stats(bool track_cpu, bool track_mem, bool track_threads, bool track_net,
                         bool show_affinity, bool track_vmem, [[maybe_unused]] const TuiColors& c) {
    std::ostringstream out;
    if (track_cpu || track_mem || track_threads || show_affinity || track_vmem) {
        // Layout: CPU, memory, optional virtual memory, thread count and core affinity
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
        // Display cumulative download (D) and upload (U) usage
        out << "Net: D " << format_bytes(usage.download_bytes) << "  U "
            << format_bytes(usage.upload_bytes) << "\n";
    }
    return out.str();
}

/**
 * @brief Render a single repository entry for the status table.
 *
 * @param p                Path to the repository.
 * @param ri               Status information for the repository.
 * @param show_skipped     Include repositories marked as skipped.
 * @param show_notgit      Include paths that are not Git repositories.
 * @param show_commit_date Display last commit date when true.
 * @param show_commit_author Display commit author when true.
 * @param session_dates_only Show commit date only for repos pulled this session.
 * @param censor_names     Obfuscate repository names.
 * @param censor_char      Replacement character for censored names.
 * @param c                ANSI color palette for highlighting.
 * @return Formatted table row or empty string if filtered out.
 */
std::string render_repo_entry(const fs::path& p, const RepoInfo& ri, bool show_skipped,
                              bool show_notgit, bool show_commit_date, bool show_commit_author,
                              bool session_dates_only, bool censor_names, char censor_char,
                              const TuiColors& c) {
    if ((ri.status == RS_SKIPPED && !show_skipped) || (ri.status == RS_NOT_GIT && !show_notgit))
        return "";
    // Default to a gray "Pending" state
    std::string color = c.gray, status_s = "Pending ";
    // Map repository status to color and display label
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
    // Status column uses a fixed width for alignment
    out << color << " [" << std::left << std::setw(9) << status_s << "]  " << name << c.reset;
    if (!ri.branch.empty()) {
        // Append branch and optional commit hash
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
        out << c.red << " [AUTH]" << c.reset; // Highlight authentication problems
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
              const TuiTheme& theme, const std::string& status_msg, int runtime_sec,
              bool show_datetime_line, bool show_header, bool show_repo_count, bool censor_names,
              char censor_char) {
    // Determine which ANSI color codes to use based on options
    TuiColors colors = make_tui_colors(no_colors, custom_color, theme);
    std::ostringstream out;
    out << render_header(all_repos, repo_infos, interval, seconds_left, scanning, action,
                         show_version, show_repo_count, status_msg, runtime_sec, show_datetime_line,
                         colors);
    out << render_stats(track_cpu, track_mem, track_threads, track_net, show_affinity, track_vmem,
                        colors);
    if (show_header) {
        // Draw table header with a fixed-width status column
        out << "--------------------------------------------------------------";
        out << "-------------------\n"; // Top border
        out << colors.bold << " [" << std::left << std::setw(9) << "Status" << "]  Repo"
            << colors.reset << "\n"; // Column titles
        out << "--------------------------------------------------------------";
        out << "-------------------\n"; // Bottom border
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
