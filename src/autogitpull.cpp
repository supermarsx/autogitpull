#include <filesystem>
#include <iostream>

#include "options.hpp"
#include "help_text.hpp"
#include "ui_loop.hpp"
#include "process_monitor.hpp"
#include "git_utils.hpp"
#include "lock_utils.hpp"
#include "version.hpp"
#ifndef _WIN32
#include "linux_daemon.hpp"
#else
#include "windows_service.hpp"
#endif

namespace fs = std::filesystem;

#ifndef AUTOGITPULL_NO_MAIN
int main(int argc, char* argv[]) {
    git::GitInitGuard git_guard;
    try {
        Options opts = parse_options(argc, argv);
        if (opts.pull_timeout.count() > 0)
            git::set_libgit_timeout(static_cast<unsigned int>(opts.pull_timeout.count()));
        if (opts.show_help) {
            print_help(argv[0]);
            return 0;
        }
        if (opts.print_version) {
            std::cout << AUTOGITPULL_VERSION << "\n";
            return 0;
        }
        if (opts.show_service) {
#ifndef _WIN32
            if (procutil::service_unit_exists(opts.daemon_name))
                std::cout << opts.daemon_name << "\n";
#else
            if (winservice::service_exists(opts.service_name))
                std::cout << opts.service_name << "\n";
#endif
            return 0;
        }
        if (opts.remove_lock) {
            if (!opts.root.empty()) {
                fs::path lock = opts.root / ".autogitpull.lock";
                procutil::release_lock_file(lock);
            }
            return 0;
        }
        if (opts.kill_all) {
            if (opts.root.empty()) {
                std::cerr << "--kill-all requires a root path" << std::endl;
                return 1;
            }
            fs::path lock = opts.root / ".autogitpull.lock";
            unsigned long pid = 0;
            if (procutil::read_lock_pid(lock, pid) && procutil::process_running(pid)) {
                if (procutil::terminate_process(pid)) {
                    procutil::release_lock_file(lock);
                    std::cout << "Terminated process " << pid << "\n";
                } else {
                    std::cerr << "Failed to terminate process " << pid << "\n";
                    return 1;
                }
            } else {
                std::cout << "No running instance" << std::endl;
            }
            return 0;
        }
        if (opts.list_instances) {
            auto insts = procutil::find_running_instances();
            for (const auto& [name, pid] : insts)
                std::cout << name << " " << pid << "\n";
            return 0;
        }
        return run_with_monitor(opts);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
#endif // AUTOGITPULL_NO_MAIN
