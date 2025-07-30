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
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

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

using namespace ftxui;

void draw_tui(const std::vector<fs::path>& all_repos,
              const std::map<fs::path, RepoInfo>& repo_infos, int interval, int seconds_left,
              bool scanning, const std::string& action, bool show_skipped, bool show_version,
              bool track_cpu, bool track_mem, bool track_threads, bool track_net,
              bool show_affinity, bool track_vmem, bool show_commit_date, bool show_commit_author,
              bool session_dates_only, bool no_colors, const std::string& custom_color,
              int runtime_sec, bool show_datetime_line, bool show_header, bool show_repo_count) {
    (void)track_cpu;
    (void)track_mem;
    (void)track_threads;
    (void)track_net;
    (void)show_affinity;
    (void)track_vmem;
    (void)show_commit_date;
    (void)show_commit_author;
    (void)session_dates_only;

    std::vector<Element> lines;
    lines.push_back(text("AutoGitPull TUI") | bold);
    if (show_version)
        lines.back() =
            hbox({text("AutoGitPull TUI"), text(std::string(" v") + AUTOGITPULL_VERSION) | bold});
    if (show_datetime_line)
        lines.push_back(text("Date: " + timestamp()));
    if (!all_repos.empty())
        lines.push_back(text("Monitoring: " + all_repos[0].parent_path().string()));
    if (show_repo_count)
        lines.push_back(text("Repos: " + std::to_string(all_repos.size())));
    lines.push_back(text("Interval: " + std::to_string(interval) + "s (Ctrl+C to exit)"));
    std::string status = scanning ? action : "Idle";
    lines.push_back(
        text("Status: " + status + " - Next scan in " + std::to_string(seconds_left) + "s"));
    if (runtime_sec >= 0)
        lines.back() =
            hbox({text("Status: " + status),
                  text(" - Next scan in " + std::to_string(seconds_left) + "s"),
                  text(" - Runtime " + format_duration_short(std::chrono::seconds(runtime_sec)))});
    if (show_header)
        lines.push_back(separator());
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
        std::string status_s = "Pending";
        Color color = Color::GrayLight;
        switch (ri.status) {
        case RS_CHECKING:
            status_s = "Checking";
            color = Color::CyanLight;
            break;
        case RS_UP_TO_DATE:
            status_s = "UpToDate";
            color = Color::GreenLight;
            break;
        case RS_PULLING:
            status_s = "Pulling";
            color = Color::YellowLight;
            break;
        case RS_PULL_OK:
            status_s = "Pulled";
            color = Color::GreenLight;
            break;
        case RS_PKGLOCK_FIXED:
            status_s = "PkgLockOk";
            color = Color::YellowLight;
            break;
        case RS_ERROR:
            status_s = "Error";
            color = Color::RedLight;
            break;
        case RS_SKIPPED:
            status_s = "Skipped";
            color = Color::GrayLight;
            break;
        case RS_HEAD_PROBLEM:
            status_s = "HEAD/BR";
            color = Color::RedLight;
            break;
        case RS_DIRTY:
            status_s = "Dirty";
            color = Color::RedLight;
            break;
        case RS_REMOTE_AHEAD:
            status_s = "RemoteUp";
            color = Color::MagentaLight;
            break;
        default:
            break;
        }
        std::string line = "[" + status_s + "]  " + p.filename().string();
        lines.push_back(text(line) | ftxui::color(color));
    }
    if (show_header)
        lines.push_back(separator());
    Element doc = vbox(std::move(lines));
    auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(doc));
    Render(screen, doc);
    std::cout << "\033[2J\033[H" << screen.ToString() << std::flush;
}
