#ifndef TUI_HPP
#define TUI_HPP

#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include "repo.hpp"

/**
 * @brief Enable ANSI color sequences on Windows consoles.
 *
 * Has no effect on other platforms.
 */
void enable_win_ansi();

/**
 * @brief Resolved color codes for the TUI.
 */
struct TuiColors {
    std::string reset;
    std::string green;
    std::string yellow;
    std::string red;
    std::string cyan;
    std::string gray;
    std::string bold;
    std::string magenta;
};

/**
 * @brief Create a color palette honoring user preferences.
 */
TuiColors make_tui_colors(bool no_colors, const std::string& custom_color);

std::string render_header(const std::vector<std::filesystem::path>& all_repos,
                          const std::map<std::filesystem::path, RepoInfo>& repo_infos, int interval,
                          int seconds_left, bool scanning, const std::string& action,
                          bool show_version, bool show_repo_count, const std::string& status_msg,
                          int runtime_sec, bool show_datetime_line, const TuiColors& colors);

std::string render_stats(bool track_cpu, bool track_mem, bool track_threads, bool track_net,
                         bool show_affinity, bool track_vmem, const TuiColors& colors);

std::string render_repo_entry(const std::filesystem::path& p, const RepoInfo& ri, bool show_skipped,
                              bool show_notgit, bool show_commit_date, bool show_commit_author,
                              bool session_dates_only, bool censor_names, char censor_char,
                              const TuiColors& colors);

/**
 * @brief Render the text user interface.
 *
 * @param all_repos   List of repository paths being monitored.
 * @param repo_infos  Map with status information for each repository.
 * @param interval    Scan interval in seconds.
 * @param seconds_left Seconds remaining until the next scan.
 * @param scanning    Whether a scan is currently in progress.
 * @param action      Short description of the current action.
 * @param show_skipped Show entries marked as skipped.
 * @param show_notgit Show entries marked as NotGit.
 */
void draw_tui(const std::vector<std::filesystem::path>& all_repos,
              const std::map<std::filesystem::path, RepoInfo>& repo_infos, int interval,
              int seconds_left, bool scanning, const std::string& action, bool show_skipped,
              bool show_notgit, bool show_version, bool track_cpu, bool track_mem,
              bool track_threads, bool track_net, bool show_affinity, bool track_vmem,
              bool show_commit_date, bool show_commit_author, bool session_dates_only,
              bool no_colors, const std::string& custom_color, const std::string& status_msg,
              int runtime_sec, bool show_datetime_line, bool show_header, bool show_repo_count,
              bool censor_names, char censor_char);

#endif // TUI_HPP
