// options_behavior.cpp
//
// Parse behavior, safety, and auxiliary flags.

#include <climits>
#include <map>
#include <string>

#include "arg_parser.hpp"
#include "options.hpp"
#include "parse_utils.hpp"

void parse_behavior_flags(Options& opts, ArgParser& parser,
                          const std::function<bool(const std::string&)>& cfg_flag,
                          const std::function<std::string(const std::string&)>& cfg_opt,
                          const std::map<std::string, std::string>& cfg_opts) {
    bool ok = false;
    opts.silent = parser.has_flag("--silent") || cfg_flag("--silent");
    opts.recursive_scan = parser.has_flag("--recursive") || cfg_flag("--recursive");
    opts.show_help = parser.has_flag("--help");
    opts.print_version = parser.has_flag("--version");
    opts.hard_reset = parser.has_flag("--hard-reset") || cfg_flag("--hard-reset");
    opts.confirm_reset = parser.has_flag("--confirm-reset") || cfg_flag("--confirm-reset");
    opts.confirm_alert = parser.has_flag("--confirm-alert") || cfg_flag("--confirm-alert");
    opts.sudo_su = parser.has_flag("--sudo-su") || cfg_flag("--sudo-su");
    opts.mutant_mode = parser.has_flag("--mutant") || parser.has_flag("--recover-mutant") ||
                       cfg_flag("--mutant") || cfg_flag("--recover-mutant");
    opts.confirm_mutant = parser.has_flag("--confirm-mutant") || cfg_flag("--confirm-mutant");
    opts.recover_mutant = parser.has_flag("--recover-mutant") || cfg_flag("--recover-mutant");
    if (parser.has_flag("--mutant-config") || cfg_opts.count("--mutant-config")) {
        std::string val = parser.get_option("--mutant-config");
        if (val.empty())
            val = cfg_opt("--mutant-config");
        if (!val.empty())
            opts.mutant_config = val;
    }
    if (opts.mutant_mode) {
        if (!(opts.confirm_mutant || opts.sudo_su))
            throw std::runtime_error("--mutant requires --confirm-mutant or --sudo-su");
        opts.service.persist = true;
        if (opts.recover_mutant) {
            opts.service.reattach = true;
            if (opts.service.attach_name.empty())
                opts.service.attach_name = "mutant";
        } else {
            opts.service.run_background = true;
            if (opts.service.attach_name.empty())
                opts.service.attach_name = "mutant";
        }
    }
    opts.add_ignore = parser.has_flag("--add-ignore") || cfg_flag("--add-ignore");
    if (opts.add_ignore)
        opts.add_ignore_repo = parser.get_option("--add-ignore");
    opts.remove_ignore = parser.has_flag("--remove-ignore") || cfg_flag("--remove-ignore");
    if (opts.remove_ignore)
        opts.remove_ignore_repo = parser.get_option("--remove-ignore");
    opts.clear_ignores = parser.has_flag("--clear-ignores") || cfg_flag("--clear-ignores");
    opts.find_ignores = parser.has_flag("--find-ignores") || cfg_flag("--find-ignores");
    if (parser.has_flag("--depth") || cfg_opts.count("--depth")) {
        std::string val = parser.get_option("--depth");
        if (val.empty())
            val = cfg_opt("--depth");
        unsigned int d = parse_uint(val, 0, UINT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --depth");
        opts.depth = d;
    }
}
