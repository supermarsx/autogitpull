#include "config_utils.hpp"
#if __has_include(<yaml-cpp/yaml.h>)
#include <yaml-cpp/yaml.h>
#define HAVE_YAMLCPP 1
#endif
#include <fstream>
#include <string>
#include <sstream>
#include <nlohmann/json.hpp>
#ifdef HAVE_YAMLCPP
static std::string yaml_node_to_string(const YAML::Node& node) {
    if (node.IsScalar()) {
        try {
            bool b = node.as<bool>();
            return b ? "true" : "false";
        } catch (...) {
        }
        try {
            long long i = node.as<long long>();
            return std::to_string(i);
        } catch (...) {
        }
        try {
            double d = node.as<double>();
            std::ostringstream oss;
            oss << d;
            return oss.str();
        } catch (...) {
        }
        return node.as<std::string>();
    } else if (node.IsNull()) {
        return "";
    } else if (node.IsDefined() && !node.IsSequence() && !node.IsMap()) {
        try {
            long long i = node.as<long long>();
            return std::to_string(i);
        } catch (...) {
        }
        try {
            double d = node.as<double>();
            std::ostringstream oss;
            oss << d;
            return oss.str();
        } catch (...) {
        }
        return node.as<std::string>();
    }
    return "";
}
#endif

bool load_yaml_config(const std::string& path, std::map<std::string, std::string>& opts,
                      std::map<std::string, std::map<std::string, std::string>>& repo_opts,
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
            const std::string key_name = it->first.as<std::string>();
            const YAML::Node& node = it->second;
            if (node.IsMap()) {
                if (key_name.find('/') != std::string::npos ||
                    key_name.find('\\') != std::string::npos) {
                    auto& m = repo_opts[key_name];
                    for (auto it2 = node.begin(); it2 != node.end(); ++it2) {
                        if (!it2->first.IsScalar())
                            continue;
                        std::string subk = "--" + it2->first.as<std::string>();
                        const YAML::Node& val = it2->second;
                        if (val.IsScalar() ||
                            (val.IsDefined() && !val.IsSequence() && !val.IsMap()))
                            m[subk] = yaml_node_to_string(val);
                        else if (val.IsNull())
                            m[subk] = "";
                    }
                } else {
                    for (auto it2 = node.begin(); it2 != node.end(); ++it2) {
                        if (!it2->first.IsScalar())
                            continue;
                        std::string key = "--" + it2->first.as<std::string>();
                        const YAML::Node& val = it2->second;
                        if (val.IsScalar() ||
                            (val.IsDefined() && !val.IsSequence() && !val.IsMap()))
                            opts[key] = yaml_node_to_string(val);
                        else if (val.IsNull())
                            opts[key] = "";
                    }
                }
            } else if (node.IsScalar()) {
                opts["--" + key_name] = yaml_node_to_string(node);
            } else if (node.IsNull()) {
                opts["--" + key_name] = "";
            } else if (node.IsDefined() && !node.IsSequence() && !node.IsMap()) {
                opts["--" + key_name] = yaml_node_to_string(node);
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
                      std::map<std::string, std::map<std::string, std::string>>& repo_opts,
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
            const std::string key_name = it.key();
            if (val.is_object()) {
                if (key_name.find('/') != std::string::npos ||
                    key_name.find('\\') != std::string::npos) {
                    auto& m = repo_opts[key_name];
                    for (auto sub = val.begin(); sub != val.end(); ++sub) {
                        std::string key = "--" + sub.key();
                        const auto& v = sub.value();
                        if (v.is_string()) {
                            m[key] = v.get<std::string>();
                        } else if (v.is_boolean()) {
                            m[key] = v.get<bool>() ? "true" : "false";
                        } else if (v.is_number_integer()) {
                            m[key] = std::to_string(v.get<long long>());
                        } else if (v.is_number_unsigned()) {
                            m[key] = std::to_string(v.get<unsigned long long>());
                        } else if (v.is_number_float()) {
                            std::ostringstream oss;
                            oss << v.get<double>();
                            m[key] = oss.str();
                        } else if (v.is_null()) {
                            m[key] = "";
                        }
                    }
                } else {
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
                            std::ostringstream oss;
                            oss << v.get<double>();
                            opts[key] = oss.str();
                        } else if (v.is_null()) {
                            opts[key] = "";
                        }
                    }
                }
            } else {
                std::string key = "--" + key_name;
                if (val.is_string()) {
                    opts[key] = val.get<std::string>();
                } else if (val.is_boolean()) {
                    opts[key] = val.get<bool>() ? "true" : "false";
                } else if (val.is_number_integer()) {
                    opts[key] = std::to_string(val.get<long long>());
                } else if (val.is_number_unsigned()) {
                    opts[key] = std::to_string(val.get<unsigned long long>());
                } else if (val.is_number_float()) {
                    std::ostringstream oss;
                    oss << val.get<double>();
                    opts[key] = oss.str();
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
