#include "cli_commands.hpp"

#ifdef _WIN32
#include "windows_service.hpp"

namespace cli::platform {

std::string service_name(const Options& opts) { return opts.service.service_name; }
std::string daemon_name(const Options& opts) { return opts.service.service_name; }

bool service_exists(const std::string& name) { return winservice::service_exists(name); }

std::vector<std::pair<std::string, ServiceStatus>> list_services() {
    std::vector<std::pair<std::string, ServiceStatus>> result;
    for (const auto& [name, st] : winservice::list_installed_services())
        result.push_back({name, {st.exists, st.running}});
    return result;
}

bool install_service(const std::string& name, const std::string& exec_path,
                     const std::string& config_file, bool persist, const std::string&) {
    return winservice::install_service(name, exec_path, config_file, persist);
}

bool uninstall_service(const std::string& name) { return winservice::uninstall_service(name); }

ServiceStatus service_status(const std::string& name) {
    winservice::ServiceStatus st{};
    winservice::service_status(name, st);
    return {st.exists, st.running};
}

bool start_service(const std::string& name) { return winservice::start_service(name); }

bool stop_service(const std::string& name, bool force) {
    return winservice::stop_service(name, force);
}

bool restart_service(const std::string& name, bool force) {
    return winservice::restart_service(name, force);
}

bool start_daemon(const std::string& name) { return winservice::start_service(name); }

bool stop_daemon(const std::string& name, bool force) {
    return winservice::stop_service(name, force);
}

bool restart_daemon(const std::string& name, bool force) {
    return winservice::restart_service(name, force);
}

} // namespace cli::platform
#endif
