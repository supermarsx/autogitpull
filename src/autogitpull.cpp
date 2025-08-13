#include <filesystem>
#include <iostream>

#include "options.hpp"
#include "help_text.hpp"
#include "ui_loop.hpp"
#include "process_monitor.hpp"
#include "git_utils.hpp"
#include "lock_utils.hpp"
#include "history_utils.hpp"
#include "version.hpp"
#ifndef _WIN32
#ifdef __APPLE__
#include "macos_daemon.hpp"
#else
#include "linux_daemon.hpp"
#endif
#else
#include "windows_service.hpp"
#endif

namespace fs = std::filesystem;

static void perform_hard_reset(const Options& opts) {
    std::error_code ec;
    if (!opts.log_file.empty())
        fs::remove(opts.log_file, ec);
    if (!opts.log_dir.empty())
        fs::remove_all(opts.log_dir, ec);
    if (!opts.root.empty()) {
        fs::remove(opts.root / ".autogitpull.lock", ec);
        fs::remove(opts.root / ".autogitpull.yaml", ec);
        fs::remove(opts.root / ".autogitpull.json", ec);
        fs::path hist = opts.root / opts.history_file;
        fs::remove(hist, ec);
    }
}

#ifndef AUTOGITPULL_NO_MAIN
int main(int argc, char* argv[]) {
    git::GitInitGuard git_guard;
    try {
        Options opts = parse_options(argc, argv);
        if (opts.enable_history) {
            std::string cmd;
            for (int i = 1; i < argc; ++i) {
                if (i > 1)
                    cmd += ' ';
                cmd += argv[i];
            }
            append_history(opts.history_file, cmd);
        }
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
        if (opts.list_services) {
#ifndef _WIN32
            auto svcs = procutil::list_installed_services();
#else
            auto svcs = winservice::list_installed_services();
#endif
            for (const auto& [name, st] : svcs)
                std::cout << name << " " << (st.running ? "running" : "stopped") << "\n";
            return 0;
        }
        if (opts.install_service || opts.install_daemon) {
#ifndef _WIN32
            std::string exec = fs::absolute(argv[0]).string();
            std::string cfg = opts.install_service ? opts.service_config : opts.daemon_config;
            std::string name = opts.install_service ? opts.service_name : opts.daemon_name;
            const char* user = std::getenv("USER");
            std::string usr = user ? user : "root";
            return procutil::create_service_unit(name, exec, cfg, usr, opts.persist) ? 0 : 1;
#else
            std::string exec = fs::absolute(argv[0]).string();
            std::string cfg = opts.install_service ? opts.service_config : opts.daemon_config;
            std::string name = opts.install_service ? opts.service_name : opts.daemon_name;
            return winservice::install_service(name, exec, cfg, opts.persist) ? 0 : 1;
#endif
        }
        if (opts.uninstall_service || opts.uninstall_daemon) {
#ifndef _WIN32
            std::string name = opts.uninstall_service ? opts.service_name : opts.daemon_name;
            return procutil::remove_service_unit(name) ? 0 : 1;
#else
            std::string name = opts.uninstall_service ? opts.service_name : opts.daemon_name;
            return winservice::uninstall_service(name) ? 0 : 1;
#endif
        }
        if (opts.service_status) {
#ifndef _WIN32
            procutil::ServiceStatus st{};
            procutil::service_unit_status(opts.daemon_name, st);
#else
            winservice::ServiceStatus st{};
            winservice::service_status(opts.service_name, st);
#endif
            std::cout << (st.exists ? "exists" : "missing") << " "
                      << (st.running ? "running" : "stopped") << "\n";
            return 0;
        }
        if (opts.daemon_status) {
#ifndef _WIN32
            procutil::ServiceStatus st{};
            procutil::service_unit_status(opts.daemon_name, st);
            std::cout << (st.exists ? "exists" : "missing") << " "
                      << (st.running ? "running" : "stopped") << "\n";
            return 0;
#else
            winservice::ServiceStatus st{};
            winservice::service_status(opts.service_name, st);
            std::cout << (st.exists ? "exists" : "missing") << " "
                      << (st.running ? "running" : "stopped") << "\n";
            return 0;
#endif
        }
#define SERVICE_NAME(name) ((name).empty() ? opts.service_name : (name))
#define DAEMON_NAME(name) ((name).empty() ? opts.daemon_name : (name))
        if (opts.start_service) {
#ifndef _WIN32
            return procutil::start_service_unit(DAEMON_NAME(opts.start_service_name)) ? 0 : 1;
#else
            return winservice::start_service(SERVICE_NAME(opts.start_service_name)) ? 0 : 1;
#endif
        }
        if (opts.stop_service) {
#ifndef _WIN32
            return procutil::stop_service_unit(DAEMON_NAME(opts.stop_service_name),
                                               opts.force_stop_service)
                       ? 0
                       : 1;
#else
            return winservice::stop_service(SERVICE_NAME(opts.stop_service_name),
                                            opts.force_stop_service)
                       ? 0
                       : 1;
#endif
        }
        if (opts.restart_service) {
#ifndef _WIN32
            return procutil::restart_service_unit(DAEMON_NAME(opts.restart_service_name),
                                                  opts.force_restart_service)
                       ? 0
                       : 1;
#else
            return winservice::restart_service(SERVICE_NAME(opts.restart_service_name),
                                               opts.force_restart_service)
                       ? 0
                       : 1;
#endif
        }
#undef SERVICE_NAME
#undef DAEMON_NAME
#ifndef _WIN32
        if (opts.start_daemon)
            return procutil::start_service_unit(
                       opts.start_daemon_name.empty() ? opts.daemon_name : opts.start_daemon_name)
                       ? 0
                       : 1;
        if (opts.stop_daemon)
            return procutil::stop_service_unit(
                       opts.stop_daemon_name.empty() ? opts.daemon_name : opts.stop_daemon_name,
                       opts.force_stop_daemon)
                       ? 0
                       : 1;
        if (opts.restart_daemon)
            return procutil::restart_service_unit(opts.restart_daemon_name.empty()
                                                      ? opts.daemon_name
                                                      : opts.restart_daemon_name,
                                                  opts.force_restart_daemon)
                       ? 0
                       : 1;
#endif
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
        if (opts.hard_reset) {
            std::cerr << "WARNING: --hard-reset will delete all application data" << std::endl;
            if (!opts.confirm_reset) {
                std::cerr << "Re-run with --confirm-reset to proceed" << std::endl;
                return 1;
            }
            perform_hard_reset(opts);
            std::cout << "Reset complete" << std::endl;
            return 0;
        }
        if (opts.list_instances) {
            auto insts = procutil::find_running_instances();
            for (const auto& [name, pid] : insts)
                std::cout << name << " " << pid << "\n";
            return 0;
        }
        if (!alerts_allowed(opts)) {
            std::cerr << "WARNING: --interval below 15s or --force-pull used" << std::endl;
            std::cerr << "Re-run with --confirm-alert or --sudo-su to proceed" << std::endl;
            return 1;
        }
        return run_with_monitor(opts);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
#endif // AUTOGITPULL_NO_MAIN
