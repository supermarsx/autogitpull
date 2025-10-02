// options_config.cpp
//
// Load configuration from YAML/JSON and auto-discovery.

#include <algorithm>
#include <filesystem>
#include <map>
#include <string>

#include "arg_parser.hpp"
#include "config_utils.hpp"
#include "options.hpp"

namespace fs = std::filesystem;

void load_config_and_auto(int argc, char* argv[], std::map<std::string, std::string>& cfg_opts,
                          std::map<std::string, std::map<std::string, std::string>>& cfg_repo_opts,
                          fs::path& config_file) {
    const std::set<std::string> pre_known{"--config-yaml", "--config-json"};
    const std::map<char, std::string> pre_short{{'y', "--config-yaml"}, {'j', "--config-json"}};
    ArgParser pre_parser(argc, argv, pre_known, pre_short);
    if (pre_parser.has_flag("--config-yaml")) {
        std::string cfg = pre_parser.get_option("--config-yaml");
        if (cfg.empty())
            throw std::runtime_error("--config-yaml requires a file");
        std::string err;
        if (!load_yaml_config(cfg, cfg_opts, cfg_repo_opts, err))
            throw std::runtime_error("Failed to load config: " + err);
        config_file = cfg;
    }
    if (pre_parser.has_flag("--config-json")) {
        std::string cfg = pre_parser.get_option("--config-json");
        if (cfg.empty())
            throw std::runtime_error("--config-json requires a file");
        std::string err;
        if (!load_json_config(cfg, cfg_opts, cfg_repo_opts, err))
            throw std::runtime_error("Failed to load config: " + err);
        config_file = cfg;
    }

    const std::set<std::string> auto_known{"--root", "--auto-config"};
    const std::map<char, std::string> auto_short{{'o', "--root"}};
    ArgParser auto_parser(argc, argv, auto_known, auto_short);
    auto cfg_flag_pre = [&](const std::string& k) {
        auto it = cfg_opts.find(k);
        if (it == cfg_opts.end())
            return false;
        std::string v = it->second;
        std::transform(v.begin(), v.end(), v.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return v == "" || v == "1" || v == "true" || v == "yes";
    };
    bool want_auto = auto_parser.has_flag("--auto-config") || cfg_flag_pre("--auto-config");
    fs::path root_hint;
    if (auto_parser.has_flag("--root"))
        root_hint = auto_parser.get_option("--root");
    else if (!auto_parser.positional().empty())
        root_hint = auto_parser.positional().front();
    if (root_hint.empty() && cfg_opts.count("--root"))
        root_hint = cfg_opts["--root"];

    if (want_auto) {
        auto find_cfg = [](const fs::path& dir) -> fs::path {
            if (dir.empty())
                return {};
            fs::path y = dir / ".autogitpull.yaml";
            if (fs::exists(y))
                return y;
            fs::path j = dir / ".autogitpull.json";
            if (fs::exists(j))
                return j;
            return {};
        };
        fs::path exe_dir;
        if (argv && argv[0])
            exe_dir = fs::absolute(argv[0]).parent_path();
        fs::path cfg_path = find_cfg(root_hint);
        if (cfg_path.empty())
            cfg_path = find_cfg(fs::current_path());
        if (cfg_path.empty())
            cfg_path = find_cfg(exe_dir);
        if (!cfg_path.empty()) {
            std::string err;
            if (cfg_path.extension() == ".yaml") {
                if (!load_yaml_config(cfg_path.string(), cfg_opts, cfg_repo_opts, err))
                    throw std::runtime_error("Failed to load config: " + err);
            } else {
                if (!load_json_config(cfg_path.string(), cfg_opts, cfg_repo_opts, err))
                    throw std::runtime_error("Failed to load config: " + err);
            }
            config_file = cfg_path;
        }
    }
}
