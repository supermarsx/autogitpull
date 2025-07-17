#ifndef CONFIG_UTILS_HPP
#define CONFIG_UTILS_HPP
#include <map>
#include <string>

bool load_yaml_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::string& error);

bool load_json_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::string& error);

#endif // CONFIG_UTILS_HPP
