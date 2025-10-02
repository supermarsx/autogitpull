// options_service.cpp
//
// Service/daemon CLI parsing helpers split from options.cpp.
// Keeps implementation units small and focused (<=300 LOC) and documents
// behavior clearly per the contributor guidelines.

#include <map>
#include <functional>
#include <string>

#include "arg_parser.hpp"
#include "options.hpp"

/**
 * Parse service and daemon related options from CLI and config.
 *
 * - Supports install/uninstall, start/stop/restart for both service and daemon
 * - Handles attach/background/reattach naming validation
 * - Merges CLI flags with config precedence provided by cfg_flag/cfg_opt
 */
void parse_service_options(Options& opts, ArgParser& parser,
                           const std::function<bool(const std::string&)>& cfg_flag,
                           const std::function<std::string(const std::string&)>& cfg_opt,
                           const std::map<std::string, std::string>& cfg_opts) {
    struct ControlFlag {
        const char* flag;
        bool ServiceOptions::*state;
        std::string ServiceOptions::*name;
    };
    struct BoolFlag {
        const char* flag;
        bool ServiceOptions::*state;
    };
    const BoolFlag bool_flags[] = {
        {"--install-daemon", &ServiceOptions::install_daemon},
        {"--uninstall-daemon", &ServiceOptions::uninstall_daemon},
        {"--install-service", &ServiceOptions::install_service},
        {"--uninstall-service", &ServiceOptions::uninstall_service},
        {"--service-status", &ServiceOptions::service_status},
        {"--daemon-status", &ServiceOptions::daemon_status},
        {"--show-service", &ServiceOptions::show_service},
        {"--kill-all", &ServiceOptions::kill_all},
        {"--kill-on-sleep", &ServiceOptions::kill_on_sleep},
        {"--list-instances", &ServiceOptions::list_instances},
    };
    for (const auto& bf : bool_flags)
        opts.service.*(bf.state) = parser.has_flag(bf.flag) || cfg_flag(bf.flag);

    if (opts.service.install_daemon) {
        std::string val = parser.get_option("--install-daemon");
        if (!val.empty())
            opts.service.daemon_name = val;
    }
    if (opts.service.uninstall_daemon) {
        std::string val = parser.get_option("--uninstall-daemon");
        if (!val.empty())
            opts.service.daemon_name = val;
    }
    if (opts.service.install_service) {
        std::string val = parser.get_option("--install-service");
        if (!val.empty())
            opts.service.service_name = val;
    }
    if (opts.service.uninstall_service) {
        std::string val = parser.get_option("--uninstall-service");
        if (!val.empty())
            opts.service.service_name = val;
    }

    opts.service.list_services = parser.has_flag("--list-services") ||
                                 parser.has_flag("--list-daemons") || cfg_flag("--list-services") ||
                                 cfg_flag("--list-daemons");

    struct ValueOpt {
        const char* flag;
        std::string ServiceOptions::*member;
    };
    const ValueOpt value_opts[] = {
        {"--daemon-config", &ServiceOptions::daemon_config},
        {"--service-config", &ServiceOptions::service_config},
        {"--service-name", &ServiceOptions::service_name},
        {"--daemon-name", &ServiceOptions::daemon_name},
    };
    for (const auto& vo : value_opts) {
        if (parser.has_flag(vo.flag) || cfg_opts.count(vo.flag)) {
            std::string val = parser.get_option(vo.flag);
            if (val.empty())
                val = cfg_opt(vo.flag);
            opts.service.*(vo.member) = val;
        }
    }

    const ControlFlag daemon_controls[] = {
        {"--start-daemon", &ServiceOptions::start_daemon, &ServiceOptions::start_daemon_name},
        {"--stop-daemon", &ServiceOptions::stop_daemon, &ServiceOptions::stop_daemon_name},
        {"--restart-daemon", &ServiceOptions::restart_daemon, &ServiceOptions::restart_daemon_name},
    };
    for (const auto& c : daemon_controls) {
        if (parser.has_flag(c.flag) || cfg_flag(c.flag)) {
            opts.service.*(c.state) = true;
            std::string val = parser.get_option(c.flag);
            if (val.empty())
                val = cfg_opt(c.flag);
            opts.service.*(c.name) = val;
        }
    }
    const ControlFlag service_controls[] = {
        {"--start-service", &ServiceOptions::start_service, &ServiceOptions::start_service_name},
        {"--stop-service", &ServiceOptions::stop_service, &ServiceOptions::stop_service_name},
        {"--restart-service", &ServiceOptions::restart_service,
         &ServiceOptions::restart_service_name},
    };
    for (const auto& c : service_controls) {
        if (parser.has_flag(c.flag) || cfg_flag(c.flag)) {
            opts.service.*(c.state) = true;
            std::string val = parser.get_option(c.flag);
            if (val.empty())
                val = cfg_opt(c.flag);
            opts.service.*(c.name) = val;
        }
    }

    struct ForceFlag {
        const char* flag;
        bool ServiceOptions::*state;
    };
    const ForceFlag force_flags[] = {
        {"--force-stop-daemon", &ServiceOptions::force_stop_daemon},
        {"--force-restart-daemon", &ServiceOptions::force_restart_daemon},
        {"--force-stop-service", &ServiceOptions::force_stop_service},
        {"--force-restart-service", &ServiceOptions::force_restart_service},
    };
    for (const auto& ff : force_flags)
        opts.service.*(ff.state) = parser.has_flag(ff.flag) || cfg_flag(ff.flag);

    if (parser.has_flag("--attach") || cfg_opts.count("--attach")) {
        std::string val = parser.get_option("--attach");
        if (val.empty())
            val = cfg_opt("--attach");
        if (val.empty())
            throw std::runtime_error("--attach requires a name");
        opts.service.attach_name = val;
    }
    opts.service.run_background = parser.has_flag("--background") || cfg_opts.count("--background");
    if (opts.service.run_background) {
        std::string val = parser.get_option("--background");
        if (val.empty())
            val = cfg_opt("--background");
        if (val.empty())
            throw std::runtime_error("--background requires a name");
        opts.service.attach_name = val;
    }
    if (parser.has_flag("--reattach") || cfg_opts.count("--reattach")) {
        std::string val = parser.get_option("--reattach");
        if (val.empty())
            val = cfg_opt("--reattach");
        if (val.empty())
            throw std::runtime_error("--reattach requires a name");
        opts.service.attach_name = val;
        opts.service.reattach = true;
    }

    if (opts.service.start_service_name.empty())
        opts.service.start_service_name = opts.service.service_name;
    if (opts.service.stop_service_name.empty())
        opts.service.stop_service_name = opts.service.service_name;
    if (opts.service.restart_service_name.empty())
        opts.service.restart_service_name = opts.service.service_name;
    if (opts.service.start_daemon_name.empty())
        opts.service.start_daemon_name = opts.service.daemon_name;
    if (opts.service.stop_daemon_name.empty())
        opts.service.stop_daemon_name = opts.service.daemon_name;
    if (opts.service.restart_daemon_name.empty())
        opts.service.restart_daemon_name = opts.service.daemon_name;
}
