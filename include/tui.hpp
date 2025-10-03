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
 * @brief Theme definition for TUI colors.
 *
 * Contains raw ANSI sequences for each color used by the interface.
 */
struct TuiTheme {
    std::string reset = "\033[0m";
    std::string green = "\033[32m";
    std::string yellow = "\033[33m";
    std::string red = "\033[31m";
    std::string cyan = "\033[36m";
    std::string gray = "\033[90m";
    std::string bold = "\033[1m";
    std::string magenta = "\033[35m";
};

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
TuiColors make_tui_colors(bool no_colors, const std::string& custom_color, const TuiTheme& theme);

/**
 * @brief Render the header section of the TUI.
 *
 * @param all_repos    Paths of all repositories being monitored.
 * @param repo_infos   Map of repository paths to their status information.
 * @param interval     Scan interval in seconds.
 * @param seconds_left Seconds until the next scan.
 * @param scanning     Whether a scan is currently running.
 * @param action       Current action label displayed to the user.
 * @param show_version Whether to include the application version in the header.
 * @param show_repo_count Whether to display the total repository count.
 * @param status_msg   Optional status message to append.
 * @param runtime_sec  Total runtime of the program in seconds.
 * @param show_datetime_line Whether to append the current date/time line.
 * @param colors       Color palette used for formatting.
 * @return Colorized header string summarizing repository status.
 */
std::string render_header(const std::vector<std::filesystem::path>& all_repos,
                          const std::map<std::filesystem::path, RepoInfo>& repo_infos, int interval,
                          int seconds_left, bool scanning, const std::string& action,
                          bool show_version, bool show_repo_count, const std::string& status_msg,
                          int runtime_sec, bool show_datetime_line, const TuiColors& colors);

/**
 * @brief Render system resource statistics.
 *
 * @param track_cpu      Whether CPU usage is tracked.
 * @param track_mem      Whether memory usage is tracked.
 * @param track_threads  Whether thread count is tracked.
 * @param track_net      Whether network usage is tracked.
 * @param show_affinity  Whether to display CPU affinity information.
 * @param track_vmem     Whether virtual memory usage is tracked.
 * @param colors         Color palette used for formatting.
 * @return Colorized statistics string for the TUI footer.
 */
std::string render_stats(bool track_cpu, bool track_mem, bool track_threads, bool track_net,
                         bool show_affinity, bool track_vmem, const TuiColors& colors);

/**
 * @brief Render a single repository entry line.
 *
 * @param p                   Path of the repository.
 * @param ri                  Status information for the repository.
 * @param show_skipped        Whether to show entries marked as skipped.
 * @param show_notgit         Whether to show entries that are not Git repositories.
 * @param show_commit_date    Whether to include the last commit date.
 * @param show_commit_author  Whether to include the commit author's name.
 * @param session_dates_only  Display commit dates only for the current session.
 * @param censor_names        Whether to redact author names.
 * @param censor_char         Replacement character used when censoring names.
 * @param colors              Color palette used for formatting.
 * @return Colorized repository line for the TUI list.
 */
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
              bool no_colors, const std::string& custom_color, const TuiTheme& theme,
              const std::string& status_msg, int runtime_sec, bool show_datetime_line,
              bool show_header, bool show_repo_count, bool censor_names, char censor_char);

#endif // TUI_HPP
