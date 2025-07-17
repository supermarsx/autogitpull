#include "config_utils.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <nlohmann/json.hpp>

bool load_yaml_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::string& error) {
    try {
        YAML::Node root = YAML::LoadFile(path);
        if (!root.IsMap()) {
            error = "Root YAML node is not a map";
            return false;
        }
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (!it->first.IsScalar())
                continue;
            std::string key = "--" + it->first.as<std::string>();
            if (it->second.IsScalar()) {
                opts[key] = it->second.as<std::string>();
            } else if (it->second.IsNull()) {
                opts[key] = "";
            } else if (it->second.IsSequence() || it->second.IsMap()) {
                // Ignore complex types
            } else if (it->second.IsDefined()) {
                opts[key] = it->second.as<std::string>();
            }
        }
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
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
            std::string key = "--" + it.key();
            const auto& val = it.value();
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
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}
