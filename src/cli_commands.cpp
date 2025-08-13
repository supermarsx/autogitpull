#include <filesystem>
#include <iostream>
#include <cstdlib>

#include "cli_commands.hpp"
#include "lock_utils.hpp"
#include "process_monitor.hpp"

namespace fs = std::filesystem;

namespace cli {

namespace {
void perform_hard_reset(const Options& opts) {
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
} // namespace

std::optional<int> handle_status_queries(const Options& opts) {
    if (opts.show_service) {
        std::string name = platform::service_name(opts);
        if (platform::service_exists(name))
            std::cout << name << "\n";
        return 0;
    }
    if (opts.list_services) {
        auto svcs = platform::list_services();
        for (const auto& [name, st] : svcs)
            std::cout << name << " " << (st.running ? "running" : "stopped") << "\n";
        return 0;
    }
    if (opts.service_status) {
        auto st = platform::service_status(platform::service_name(opts));
        std::cout << (st.exists ? "exists" : "missing") << " "
                  << (st.running ? "running" : "stopped") << "\n";
        return 0;
    }
    if (opts.daemon_status) {
        auto st = platform::service_status(platform::daemon_name(opts));
        std::cout << (st.exists ? "exists" : "missing") << " "
                  << (st.running ? "running" : "stopped") << "\n";
        return 0;
    }
    if (opts.list_instances) {
        auto insts = procutil::find_running_instances();
        for (const auto& [name, pid] : insts)
            std::cout << name << " " << pid << "\n";
        return 0;
    }
    return std::nullopt;
}

std::optional<int> handle_service_control(const Options& opts, const std::string& exec_path) {
    if (opts.install_service) {
        std::string exec = fs::absolute(exec_path).string();
        const char* user_env = std::getenv("USER");
        std::string usr = user_env ? user_env : "root";
        bool ok = platform::install_service(opts.service_name, exec, opts.service_config,
                                            opts.persist, usr);
        return ok ? std::optional<int>(0) : std::optional<int>(1);
    }
    if (opts.uninstall_service)
        return platform::uninstall_service(opts.service_name) ? 0 : 1;
    if (opts.start_service) {
        std::string name =
            opts.start_service_name.empty() ? opts.service_name : opts.start_service_name;
        return platform::start_service(name) ? 0 : 1;
    }
    if (opts.stop_service) {
        std::string name =
            opts.stop_service_name.empty() ? opts.service_name : opts.stop_service_name;
        return platform::stop_service(name, opts.force_stop_service) ? 0 : 1;
    }
    if (opts.restart_service) {
        std::string name =
            opts.restart_service_name.empty() ? opts.service_name : opts.restart_service_name;
        return platform::restart_service(name, opts.force_restart_service) ? 0 : 1;
    }
    return std::nullopt;
}

std::optional<int> handle_daemon_control(const Options& opts, const std::string& exec_path) {
    if (opts.install_daemon) {
        std::string exec = fs::absolute(exec_path).string();
        const char* user_env = std::getenv("USER");
        std::string usr = user_env ? user_env : "root";
        bool ok = platform::install_service(opts.daemon_name, exec, opts.daemon_config,
                                            opts.persist, usr);
        return ok ? std::optional<int>(0) : std::optional<int>(1);
    }
    if (opts.uninstall_daemon)
        return platform::uninstall_service(opts.daemon_name) ? 0 : 1;
    if (opts.start_daemon) {
        std::string name =
            opts.start_daemon_name.empty() ? opts.daemon_name : opts.start_daemon_name;
        return platform::start_daemon(name) ? 0 : 1;
    }
    if (opts.stop_daemon) {
        std::string name = opts.stop_daemon_name.empty() ? opts.daemon_name : opts.stop_daemon_name;
        return platform::stop_daemon(name, opts.force_stop_daemon) ? 0 : 1;
    }
    if (opts.restart_daemon) {
        std::string name =
            opts.restart_daemon_name.empty() ? opts.daemon_name : opts.restart_daemon_name;
        return platform::restart_daemon(name, opts.force_restart_daemon) ? 0 : 1;
    }
    return std::nullopt;
}

std::optional<int> handle_hard_reset(const Options& opts) {
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
    return std::nullopt;
}

int handle_monitoring_run(const Options& opts) {
    if (!alerts_allowed(opts)) {
        std::cerr << "WARNING: --interval below 15s or --force-pull used" << std::endl;
        std::cerr << "Re-run with --confirm-alert or --sudo-su to proceed" << std::endl;
        return 1;
    }
    return run_with_monitor(opts);
}

} // namespace cli
