#include <filesystem>
#include <iostream>

#include "options.hpp"
#include "help_text.hpp"
#include "ui_loop.hpp"
#include "git_utils.hpp"
#include "history_utils.hpp"
#include "version.hpp"
#include "cli_commands.hpp"
#include "mutant_mode.hpp"

namespace fs = std::filesystem;

#ifndef AUTOGITPULL_NO_MAIN
int main(int argc, char* argv[]) {
    git::GitInitGuard git_guard;
    try {
        Options opts = parse_options(argc, argv);
        apply_mutant_mode(opts);
        if (opts.enable_history) {
            std::string cmd;
            for (int i = 1; i < argc; ++i) {
                if (i > 1)
                    cmd += ' ';
                cmd += argv[i];
            }
            append_history(opts.history_file, cmd);
        }
        if (opts.limits.pull_timeout.count() > 0)
            git::set_libgit_timeout(static_cast<unsigned int>(opts.limits.pull_timeout.count()));
        if (opts.show_help) {
            print_help(argv[0]);
            return 0;
        }
        if (opts.print_version) {
            std::cout << AUTOGITPULL_VERSION << "\n";
            return 0;
        }
        std::string exec_path = argv[0];
        if (auto rc = cli::handle_status_queries(opts); rc)
            return *rc;
        if (auto rc = cli::handle_service_control(opts, exec_path); rc)
            return *rc;
        if (auto rc = cli::handle_daemon_control(opts, exec_path); rc)
            return *rc;
        if (auto rc = cli::handle_hard_reset(opts); rc)
            return *rc;
        return cli::handle_monitoring_run(opts);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
#endif // AUTOGITPULL_NO_MAIN
