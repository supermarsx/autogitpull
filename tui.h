#ifndef TUI_H
#define TUI_H

#include <vector>
#include <map>
#include <filesystem>

#include "repo_info.h"

namespace tui {

void draw_tui(const std::vector<std::filesystem::path>& all_repos,
              const std::map<std::filesystem::path, git::RepoInfo>& repo_infos,
              int interval, int seconds_left, bool scanning,
              bool show_skipped);

void enable_win_ansi();

} // namespace tui

#endif // TUI_H
