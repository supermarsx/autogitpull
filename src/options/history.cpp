// options_history.cpp
//
// Apply persist/attach defaults and resolve history file.

#include <algorithm>
#include <filesystem>
#include <string>

#include "arg_parser.hpp"
#include "options.hpp"
#include "parse_utils.hpp"

namespace fs = std::filesystem;

void finalize_persist_and_history(Options& opts, ArgParser& parser,
                                  const std::function<bool(const std::string&)>& cfg_flag,
                                  const std::function<std::string(const std::string&)>& cfg_opt,
                                  const std::map<std::string, std::string>& cfg_opts,
                                  int argc, char* argv[]) {
    (void)argc;
    bool persist_flag = parser.has_flag("--persist") || cfg_flag("--persist");
    std::string persist_val;
    if (persist_flag) {
        persist_val = parser.get_option("--persist");
        if (persist_val.empty())
            persist_val = cfg_opt("--persist");
    }

    if (opts.root.empty() && !opts.show_help && !opts.print_version && !opts.service.show_service &&
        ((opts.service.attach_name.empty() && !opts.service.reattach) ||
         opts.service.run_background || persist_flag))
        throw std::runtime_error("Root path required");

    if (persist_flag) {
        opts.service.persist = true;
        std::string name = persist_val;
        if (name.empty())
            name = opts.root.filename().string();
        if (opts.service.attach_name.empty())
            opts.service.attach_name = name;
    }

    if ((opts.rerun_last || opts.save_args || opts.enable_history) &&
        opts.history_file == ".autogitpull.config") {
        auto find_hist = [](const fs::path& dir) -> fs::path {
            if (dir.empty())
                return {};
            fs::path c = dir / ".autogitpull.config";
            if (fs::exists(c))
                return c;
            return {};
        };
        fs::path exe_dir;
        if (argv && argv[0])
            exe_dir = fs::absolute(argv[0]).parent_path();
        fs::path hist = find_hist(opts.root);
        if (hist.empty())
            hist = find_hist(fs::current_path());
        if (hist.empty())
            hist = find_hist(exe_dir);
        if (!hist.empty())
            opts.history_file = hist.string();
    }
}
