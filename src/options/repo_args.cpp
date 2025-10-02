// options_repo_args.cpp
//
// Parse root path, remote/pull-ref, and include/ignore lists.

#include <filesystem>
#include <map>
#include <string>

#include "arg_parser.hpp"
#include "options.hpp"

namespace fs = std::filesystem;

void parse_root_and_repo_filters(Options& opts, ArgParser& parser,
                                 const std::function<std::string(const std::string&)>& cfg_opt,
                                 const std::map<std::string, std::string>& cfg_opts) {
    if (parser.has_flag("--remote") || cfg_opts.count("--remote")) {
        std::string val = parser.get_option("--remote");
        if (val.empty())
            val = cfg_opt("--remote");
        if (val.empty())
            throw std::runtime_error("--remote requires a name");
        opts.remote_name = val;
    }
    if (parser.has_flag("--pull-ref") || cfg_opts.count("--pull-ref")) {
        std::string val = parser.get_option("--pull-ref");
        if (val.empty())
            val = cfg_opt("--pull-ref");
        if (val.empty())
            throw std::runtime_error("--pull-ref requires a ref name or commit hash");
        opts.pull_ref = val;
    }
    if (parser.has_flag("--root") || cfg_opts.count("--root")) {
        std::string val = parser.get_option("--root");
        if (val.empty())
            val = cfg_opt("--root");
        if (val.empty())
            throw std::runtime_error("--root requires a path");
        opts.root = val;
    } else {
        opts.root = parser.positional().empty() ? fs::path() : fs::path(parser.positional().front());
    }
    for (const auto& val : parser.get_all_options("--include-dir"))
        opts.include_dirs.push_back(val);
    for (const auto& val : parser.get_all_options("--ignore"))
        opts.ignore_dirs.push_back(val);
}
