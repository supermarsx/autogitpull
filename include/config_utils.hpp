#ifndef CONFIG_UTILS_HPP
#define CONFIG_UTILS_HPP
#include <map>
#include <string>
#include "repo_options.hpp"
#include "tui.hpp"

/**
 * @brief Load configuration options from a YAML file.
 *
 * Reads both global options and repository-specific overrides from the file at
 * @p path. On success, the provided option maps are populated with any values
 * found.
 *
 * @param path      Filesystem path to the YAML configuration file.
 * @param opts      Map receiving global option values.
 * @param repo_opts Map receiving per-repository option maps keyed by repo
 *                  identifier.
 * @param error     Output string capturing a human-readable error message on
 *                  failure.
 * @return `true` if the configuration was loaded successfully; `false`
 *         otherwise.
 */
bool load_yaml_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::map<std::string, std::map<std::string, std::string>>& repo_opts,
                      std::string& error);

/**
 * @brief Load configuration options from a JSON file.
 *
 * Reads both global options and repository-specific overrides from the file at
 * @p path. On success, the provided option maps are populated with any values
 * found.
 *
 * @param path      Filesystem path to the JSON configuration file.
 * @param opts      Map receiving global option values.
 * @param repo_opts Map receiving per-repository option maps keyed by repo
 *                  identifier.
 * @param error     Output string capturing a human-readable error message on
 *                  failure.
 * @return `true` if the configuration was loaded successfully; `false`
 *         otherwise.
 */
bool load_json_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::map<std::string, std::map<std::string, std::string>>& repo_opts,
                      std::string& error);

/**
 * @brief Load a theme definition for the text user interface.
 *
 * Reads the theme file at @p path and fills the supplied theme structure with
 * the parsed values.
 *
 * @param path  Filesystem path to the theme file.
 * @param theme Output theme structure populated on success.
 * @param error Output string capturing a human-readable error message on
 *              failure.
 * @return `true` if the theme was loaded successfully; `false` otherwise.
 */
bool load_theme(const std::string& path, TuiTheme& theme, std::string& error);

#endif // CONFIG_UTILS_HPP
