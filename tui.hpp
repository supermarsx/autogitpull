#ifndef TUI_HPP
#define TUI_HPP

#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include "repo.hpp"

/**
 * @brief Enable ANSI color sequences on Windows consoles.
 *
 * Has no effect on other platforms.
 */
void enable_win_ansi();


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
 */
void draw_tui(const std::vector<std::filesystem::path> &all_repos,
              const std::map<std::filesystem::path, RepoInfo> &repo_infos, int interval,
              int seconds_left, bool scanning, const std::string &action, bool show_skipped,
              bool show_version, bool track_cpu, bool track_mem, bool track_threads);

#endif // TUI_HPP
