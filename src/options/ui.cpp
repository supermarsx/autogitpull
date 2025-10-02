// options_ui.cpp
//
// Parse logging and UI presentation related flags/options.

#include <string>
#include <map>
#include <climits>

#include "options.hpp"
#include "arg_parser.hpp"
#include "parse_utils.hpp"
#include "tui.hpp"
#include "config_utils.hpp"

void parse_logging_and_ui(Options& opts, ArgParser& parser,
                          const std::function<bool(const std::string&)>& cfg_flag,
                          const std::function<std::string(const std::string&)>& cfg_opt,
                          const std::map<std::string, std::string>& cfg_opts) {
    bool ok = false;
    if (parser.has_flag("--log-dir") || cfg_opts.count("--log-dir")) {
        std::string val = parser.get_option("--log-dir");
        if (val.empty())
            val = cfg_opt("--log-dir");
        if (val.empty())
            throw std::runtime_error("--log-dir requires a path");
        opts.logging.log_dir = val;
    }
    if (parser.has_flag("--log-file") || cfg_opts.count("--log-file")) {
        std::string val = parser.get_option("--log-file");
        if (val.empty())
            val = cfg_opt("--log-file");
        opts.logging.log_file = val;
    }
    if (parser.has_flag("--ssh-public-key") || cfg_opts.count("--ssh-public-key")) {
        std::string val = parser.get_option("--ssh-public-key");
        if (val.empty())
            val = cfg_opt("--ssh-public-key");
        if (val.empty())
            throw std::runtime_error("--ssh-public-key requires a path");
        opts.ssh_public_key = val;
    }
    if (parser.has_flag("--ssh-private-key") || cfg_opts.count("--ssh-private-key")) {
        std::string val = parser.get_option("--ssh-private-key");
        if (val.empty())
            val = cfg_opt("--ssh-private-key");
        if (val.empty())
            throw std::runtime_error("--ssh-private-key requires a path");
        opts.ssh_private_key = val;
    }
    if (parser.has_flag("--credential-file") || cfg_opts.count("--credential-file")) {
        std::string val = parser.get_option("--credential-file");
        if (val.empty())
            val = cfg_opt("--credential-file");
        if (val.empty())
            throw std::runtime_error("--credential-file requires a path");
        opts.credential_file = val;
    }
    if (parser.has_flag("--proxy") || cfg_opts.count("--proxy")) {
        std::string val = parser.get_option("--proxy");
        if (val.empty())
            val = cfg_opt("--proxy");
        if (val.empty())
            throw std::runtime_error("--proxy requires a URL");
        opts.proxy_url = val;
    }
    if (parser.has_flag("--post-pull-hook") || cfg_opts.count("--post-pull-hook")) {
        std::string val = parser.get_option("--post-pull-hook");
        if (val.empty())
            val = cfg_opt("--post-pull-hook");
        if (val.empty())
            throw std::runtime_error("--post-pull-hook requires a path");
        opts.post_pull_hook = val;
    }
    if (parser.has_flag("--max-log-size") || cfg_opts.count("--max-log-size")) {
        std::string val = parser.get_option("--max-log-size");
        if (val.empty())
            val = cfg_opt("--max-log-size");
        ok = false;
        opts.logging.max_log_size = parse_bytes(val, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --max-log-size");
    }
    opts.show_commit_date = parser.has_flag("--show-commit-date") || cfg_flag("--show-commit-date");
    opts.show_commit_author =
        parser.has_flag("--show-commit-author") || cfg_flag("--show-commit-author");
    opts.show_datetime_line =
        !(parser.has_flag("--hide-date-time") || cfg_flag("--hide-date-time"));
    opts.show_header = !(parser.has_flag("--hide-header") || cfg_flag("--hide-header"));
    opts.show_vmem = parser.has_flag("--vmem") || cfg_flag("--vmem");
    opts.no_colors = parser.has_flag("--no-colors") || cfg_flag("--no-colors");
    if (parser.has_flag("--color") || cfg_opts.count("--color")) {
        std::string val = parser.get_option("--color");
        if (val.empty())
            val = cfg_opt("--color");
        opts.custom_color = val;
    }
    if (parser.has_flag("--theme") || cfg_opts.count("--theme")) {
        std::string val = parser.get_option("--theme");
        if (val.empty())
            val = cfg_opt("--theme");
        opts.theme_file = val;
        if (!val.empty()) {
            std::string err;
            if (!load_theme(val, opts.theme, err))
                throw std::runtime_error("Failed to load theme: " + err);
        }
    }
    if (parser.has_flag("--row-order") || cfg_opts.count("--row-order")) {
        std::string val = parser.get_option("--row-order");
        if (val.empty())
            val = cfg_opt("--row-order");
        if (val == "alpha")
            opts.sort_mode = Options::ALPHA;
        else if (val == "reverse")
            opts.sort_mode = Options::REVERSE;
        else if (val == "updated")
            opts.sort_mode = Options::UPDATED;
        else
            throw std::runtime_error("Invalid value for --row-order");
    }
    opts.logging.json_log = parser.has_flag("--json-log") || cfg_flag("--json-log");
    opts.logging.compress_logs = parser.has_flag("--compress-logs") || cfg_flag("--compress-logs");
    opts.logging.use_syslog = parser.has_flag("--syslog") || cfg_flag("--syslog");
    if (parser.has_flag("--syslog-facility") || cfg_opts.count("--syslog-facility")) {
        std::string val = parser.get_option("--syslog-facility");
        if (val.empty())
            val = cfg_opt("--syslog-facility");
        ok = false;
        int fac = parse_int(val, 0, INT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --syslog-facility");
        opts.logging.syslog_facility = fac;
    }
}
