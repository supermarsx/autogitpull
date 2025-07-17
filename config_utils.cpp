#include "config_utils.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>

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
