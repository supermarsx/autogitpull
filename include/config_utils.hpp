#ifndef CONFIG_UTILS_HPP
#define CONFIG_UTILS_HPP
#include <map>
#include <string>
#include "repo_options.hpp"
#include "tui.hpp"

bool load_yaml_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::map<std::string, std::map<std::string, std::string>>& repo_opts,
                      std::string& error);

bool load_json_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::map<std::string, std::map<std::string, std::string>>& repo_opts,
                      std::string& error);

bool load_theme(const std::string& path, TuiTheme& theme, std::string& error);

#endif // CONFIG_UTILS_HPP
