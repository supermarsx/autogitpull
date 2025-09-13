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
    if (!opts.logging.log_file.empty())
        fs::remove(opts.logging.log_file, ec);
    if (!opts.logging.log_dir.empty())
        fs::remove_all(opts.logging.log_dir, ec);
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
    if (opts.service.show_service) {
        std::string name = platform::service_name(opts);
        if (platform::service_exists(name))
            std::cout << name << "\n";
        return 0;
    }
    if (opts.service.list_services) {
        auto svcs = platform::list_services();
        for (const auto& [name, st] : svcs)
            std::cout << name << " " << (st.running ? "running" : "stopped") << "\n";
        return 0;
    }
    if (opts.service.service_status) {
        auto st = platform::service_status(platform::service_name(opts));
        std::cout << (st.exists ? "exists" : "missing") << " "
                  << (st.running ? "running" : "stopped") << "\n";
        return 0;
    }
    if (opts.service.daemon_status) {
        auto st = platform::service_status(platform::daemon_name(opts));
        std::cout << (st.exists ? "exists" : "missing") << " "
                  << (st.running ? "running" : "stopped") << "\n";
        return 0;
    }
    if (opts.service.list_instances) {
        auto insts = procutil::find_running_instances();
        for (const auto& [name, pid] : insts)
            std::cout << name << " " << pid << "\n";
        return 0;
    }
    return std::nullopt;
}

std::optional<int> handle_service_control(const Options& opts, const std::string& exec_path) {
    if (opts.service.install_service) {
        std::string exec = fs::absolute(exec_path).string();
#ifdef _WIN32
        char* user_dup = nullptr; size_t user_len = 0;
        (void)_dupenv_s(&user_dup, &user_len, "USER");
        std::string usr = user_dup ? user_dup : "root";
        if (user_dup) free(user_dup);
#else
        const char* user_env = std::getenv("USER");
        std::string usr = user_env ? user_env : "root";
#endif
        bool ok = platform::install_service(opts.service.service_name, exec,
                                            opts.service.service_config, opts.service.persist, usr);
        return ok ? std::optional<int>(0) : std::optional<int>(1);
    }
    if (opts.service.uninstall_service)
        return platform::uninstall_service(opts.service.service_name) ? 0 : 1;
    if (opts.service.start_service) {
        std::string name = opts.service.start_service_name.empty()
                               ? opts.service.service_name
                               : opts.service.start_service_name;
        return platform::start_service(name) ? 0 : 1;
    }
    if (opts.service.stop_service) {
        std::string name = opts.service.stop_service_name.empty() ? opts.service.service_name
                                                                  : opts.service.stop_service_name;
        return platform::stop_service(name, opts.service.force_stop_service) ? 0 : 1;
    }
    if (opts.service.restart_service) {
        std::string name = opts.service.restart_service_name.empty()
                               ? opts.service.service_name
                               : opts.service.restart_service_name;
        return platform::restart_service(name, opts.service.force_restart_service) ? 0 : 1;
    }
    return std::nullopt;
}

std::optional<int> handle_daemon_control(const Options& opts, const std::string& exec_path) {
    if (opts.service.install_daemon) {
        std::string exec = fs::absolute(exec_path).string();
#ifdef _WIN32
        char* user_dup = nullptr; size_t user_len = 0;
        (void)_dupenv_s(&user_dup, &user_len, "USER");
        std::string usr = user_dup ? user_dup : "root";
        if (user_dup) free(user_dup);
#else
        const char* user_env = std::getenv("USER");
        std::string usr = user_env ? user_env : "root";
#endif
        bool ok = platform::install_service(opts.service.daemon_name, exec,
                                            opts.service.daemon_config, opts.service.persist, usr);
        return ok ? std::optional<int>(0) : std::optional<int>(1);
    }
    if (opts.service.uninstall_daemon)
        return platform::uninstall_service(opts.service.daemon_name) ? 0 : 1;
    if (opts.service.start_daemon) {
        std::string name = opts.service.start_daemon_name.empty() ? opts.service.daemon_name
                                                                  : opts.service.start_daemon_name;
        return platform::start_daemon(name) ? 0 : 1;
    }
    if (opts.service.stop_daemon) {
        std::string name = opts.service.stop_daemon_name.empty() ? opts.service.daemon_name
                                                                 : opts.service.stop_daemon_name;
        return platform::stop_daemon(name, opts.service.force_stop_daemon) ? 0 : 1;
    }
    if (opts.service.restart_daemon) {
        std::string name = opts.service.restart_daemon_name.empty()
                               ? opts.service.daemon_name
                               : opts.service.restart_daemon_name;
        return platform::restart_daemon(name, opts.service.force_restart_daemon) ? 0 : 1;
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
    if (opts.service.kill_all) {
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
        std::cerr << "WARNING: --hard-reset permanently removes logs, configs, and lock files"
                  << std::endl;
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
        if (opts.interval < 15)
            std::cerr << "WARNING: --interval below 15s may overwhelm remote repositories"
                      << std::endl;
        if (opts.force_pull)
            std::cerr << "WARNING: --force-pull discards uncommitted changes and untracked files"
                      << std::endl;
        std::cerr << "Re-run with --confirm-alert or --sudo-su to proceed" << std::endl;
        return 1;
    }
    return run_with_monitor(opts);
}

} // namespace cli
