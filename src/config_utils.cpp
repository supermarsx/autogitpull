#include "config_utils.hpp"
#if __has_include(<yaml-cpp/yaml.h>)
#include <yaml-cpp/yaml.h>
#define HAVE_YAMLCPP 1
#endif
#include <fstream>
#include <nlohmann/json.hpp>

bool load_yaml_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::string& error) {
#ifdef HAVE_YAMLCPP
    try {
        std::ifstream ifs(path);
        if (!ifs) {
            error = "Failed to open file";
            return false;
        }
        YAML::Node root = YAML::Load(ifs);
        if (!root.IsMap()) {
            error = "Root YAML node is not a map";
            return false;
        }
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (!it->first.IsScalar())
                continue;
            const std::string base = "--" + it->first.as<std::string>();
            const YAML::Node& node = it->second;
            if (node.IsMap()) {
                for (auto it2 = node.begin(); it2 != node.end(); ++it2) {
                    if (!it2->first.IsScalar())
                        continue;
                    std::string key = "--" + it2->first.as<std::string>();
                    const YAML::Node& val = it2->second;
                    if (val.IsScalar())
                        opts[key] = val.as<std::string>();
                    else if (val.IsNull())
                        opts[key] = "";
                    else if (val.IsDefined() && !val.IsSequence() && !val.IsMap())
                        opts[key] = val.as<std::string>();
                }
            } else if (node.IsScalar()) {
                opts[base] = node.as<std::string>();
            } else if (node.IsNull()) {
                opts[base] = "";
            } else if (node.IsDefined() && !node.IsSequence() && !node.IsMap()) {
                opts[base] = node.as<std::string>();
            }
        }
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
#else
    (void)path;
    (void)opts;
    error = "YAML support not available";
    return false;
#endif
}

bool load_json_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::string& error) {
    try {
        std::ifstream ifs(path);
        if (!ifs) {
            error = "Failed to open file";
            return false;
        }
        nlohmann::json root;
        ifs >> root;
        if (!root.is_object()) {
            error = "Root JSON value is not an object";
            return false;
        }
        for (auto it = root.begin(); it != root.end(); ++it) {
            const auto& val = it.value();
            if (val.is_object()) {
                for (auto sub = val.begin(); sub != val.end(); ++sub) {
                    std::string key = "--" + sub.key();
                    const auto& v = sub.value();
                    if (v.is_string()) {
                        opts[key] = v.get<std::string>();
                    } else if (v.is_boolean()) {
                        opts[key] = v.get<bool>() ? "true" : "false";
                    } else if (v.is_number_integer()) {
                        opts[key] = std::to_string(v.get<long long>());
                    } else if (v.is_number_unsigned()) {
                        opts[key] = std::to_string(v.get<unsigned long long>());
                    } else if (v.is_number_float()) {
                        opts[key] = std::to_string(v.get<double>());
                    } else if (v.is_null()) {
                        opts[key] = "";
                    }
                }
            } else {
                std::string key = "--" + it.key();
                if (val.is_string()) {
                    opts[key] = val.get<std::string>();
                } else if (val.is_boolean()) {
                    opts[key] = val.get<bool>() ? "true" : "false";
                } else if (val.is_number_integer()) {
                    opts[key] = std::to_string(val.get<long long>());
                } else if (val.is_number_unsigned()) {
                    opts[key] = std::to_string(val.get<unsigned long long>());
                } else if (val.is_number_float()) {
                    opts[key] = std::to_string(val.get<double>());
                } else if (val.is_null()) {
                    opts[key] = "";
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}
