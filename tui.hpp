#ifndef TUI_HPP
#define TUI_HPP

#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include "repo.hpp"

void enable_win_ansi();
std::string timestamp();
void draw_tui(const std::vector<std::filesystem::path>& all_repos,
              const std::map<std::filesystem::path, RepoInfo>& repo_infos,
              int interval, int seconds_left, bool scanning,
              const std::string& action, bool show_skipped);

#endif // TUI_HPP
