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
static bool to_string_value(const YAML::Node& node, std::string& out) {
    if (!node.IsDefined() || node.IsSequence() || node.IsMap())
        return false;
    if (node.IsNull()) {
        out.clear();
        return true;
    }
    try {
        bool b = node.as<bool>();
        out = b ? "true" : "false";
        return true;
    } catch (...) {
    }
    try {
        long long i = node.as<long long>();
        out = std::to_string(i);
        return true;
    } catch (...) {
    }
    try {
        double d = node.as<double>();
        std::ostringstream oss;
        oss << d;
        out = oss.str();
        return true;
    } catch (...) {
    }
    try {
        out = node.as<std::string>();
        return true;
    } catch (...) {
    }
    return false;
}
#endif

static bool to_string_value(const nlohmann::json& v, std::string& out) {
    if (v.is_string()) {
        out = v.get<std::string>();
        return true;
    }
    if (v.is_boolean()) {
        out = v.get<bool>() ? "true" : "false";
        return true;
    }
    if (v.is_number_integer()) {
        out = std::to_string(v.get<long long>());
        return true;
    }
    if (v.is_number_unsigned()) {
        out = std::to_string(v.get<unsigned long long>());
        return true;
    }
    if (v.is_number_float()) {
        std::ostringstream oss;
        oss << v.get<double>();
        out = oss.str();
        return true;
    }
    if (v.is_null()) {
        out.clear();
        return true;
    }
    return false;
}

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
            if (key_name == "repositories" && node.IsMap()) {
                for (auto it2 = node.begin(); it2 != node.end(); ++it2) {
                    if (!it2->first.IsScalar())
                        continue;
                    const std::string repo_key = it2->first.as<std::string>();
                    const YAML::Node& repo_node = it2->second;
                    auto& m = repo_opts[repo_key];
                    if (repo_node.IsMap()) {
                        for (auto it3 = repo_node.begin(); it3 != repo_node.end(); ++it3) {
                            if (!it3->first.IsScalar())
                                continue;
                            std::string subk = "--" + it3->first.as<std::string>();
                            const YAML::Node& val = it3->second;
                            std::string s;
                            if (to_string_value(val, s))
                                m[subk] = s;
                        }
                    } else if (repo_node.IsNull()) {
                        m.clear();
                    }
                }
            } else if (node.IsMap()) {
                if (key_name.find('/') != std::string::npos ||
                    key_name.find('\\') != std::string::npos) {
                    auto& m = repo_opts[key_name];
                    for (auto it2 = node.begin(); it2 != node.end(); ++it2) {
                        if (!it2->first.IsScalar())
                            continue;
                        std::string subk = "--" + it2->first.as<std::string>();
                        const YAML::Node& val = it2->second;
                        std::string s;
                        if (to_string_value(val, s))
                            m[subk] = s;
                    }
                } else {
                    for (auto it2 = node.begin(); it2 != node.end(); ++it2) {
                        if (!it2->first.IsScalar())
                            continue;
                        std::string key = "--" + it2->first.as<std::string>();
                        const YAML::Node& val = it2->second;
                        std::string s;
                        if (to_string_value(val, s))
                            opts[key] = s;
                    }
                }
            } else {
                std::string s;
                if (to_string_value(node, s))
                    opts["--" + key_name] = s;
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
            if (key_name == "repositories" && val.is_object()) {
                for (auto repo_it = val.begin(); repo_it != val.end(); ++repo_it) {
                    const std::string repo_key = repo_it.key();
                    const auto& repo_node = repo_it.value();
                    auto& m = repo_opts[repo_key];
                    if (repo_node.is_object()) {
                        for (auto sub = repo_node.begin(); sub != repo_node.end(); ++sub) {
                            std::string key = "--" + sub.key();
                            const auto& v = sub.value();
                            std::string s;
                            if (to_string_value(v, s))
                                m[key] = s;
                        }
                    } else if (repo_node.is_null()) {
                        m.clear();
                    }
                }
            } else if (val.is_object()) {
                if (key_name.find('/') != std::string::npos ||
                    key_name.find('\\') != std::string::npos) {
                    auto& m = repo_opts[key_name];
                    for (auto sub = val.begin(); sub != val.end(); ++sub) {
                        std::string key = "--" + sub.key();
                        const auto& v = sub.value();
                        std::string s;
                        if (to_string_value(v, s))
                            m[key] = s;
                    }
                } else {
                    for (auto sub = val.begin(); sub != val.end(); ++sub) {
                        std::string key = "--" + sub.key();
                        const auto& v = sub.value();
                        std::string s;
                        if (to_string_value(v, s))
                            opts[key] = s;
                    }
                }
            } else {
                std::string key = "--" + key_name;
                std::string s;
                if (to_string_value(val, s))
                    opts[key] = s;
            }
        }
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

static void assign_theme_field(const std::string& key, const std::string& val, TuiTheme& theme) {
    if (key == "reset")
        theme.reset = val;
    else if (key == "green")
        theme.green = val;
    else if (key == "yellow")
        theme.yellow = val;
    else if (key == "red")
        theme.red = val;
    else if (key == "cyan")
        theme.cyan = val;
    else if (key == "gray")
        theme.gray = val;
    else if (key == "bold")
        theme.bold = val;
    else if (key == "magenta")
        theme.magenta = val;
}

bool load_theme(const std::string& path, TuiTheme& theme, std::string& error) {
    std::string ext;
    auto pos = path.find_last_of('.');
    if (pos != std::string::npos)
        ext = path.substr(pos + 1);
    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    try {
        std::ifstream ifs(path);
        if (!ifs) {
            error = "Failed to open file";
            return false;
        }
        if (ext == "json") {
            nlohmann::json root;
            ifs >> root;
            if (!root.is_object()) {
                error = "Root JSON value is not an object";
                return false;
            }
            for (auto it = root.begin(); it != root.end(); ++it) {
                if (it.value().is_string())
                    assign_theme_field(it.key(), it.value().get<std::string>(), theme);
            }
            return true;
        }
#ifdef HAVE_YAMLCPP
        YAML::Node root = YAML::Load(ifs);
        if (!root.IsMap()) {
            error = "Root YAML node is not a map";
            return false;
        }
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (it->first.IsScalar() && it->second.IsScalar()) {
                assign_theme_field(it->first.as<std::string>(), it->second.as<std::string>(),
                                   theme);
            }
        }
        return true;
#else
        error = "YAML support not available";
        return false;
#endif
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}
