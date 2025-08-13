#include "cli_commands.hpp"

#ifndef _WIN32
#include "linux_daemon.hpp"
#include <cstdlib>

namespace cli::platform {

std::string service_name(const Options& opts) { return opts.service.daemon_name; }
std::string daemon_name(const Options& opts) { return opts.service.daemon_name; }

bool service_exists(const std::string& name) { return procutil::service_unit_exists(name); }

std::vector<std::pair<std::string, ServiceStatus>> list_services() {
    std::vector<std::pair<std::string, ServiceStatus>> result;
    for (const auto& [name, st] : procutil::list_installed_services())
        result.push_back({name, {st.exists, st.running}});
    return result;
}

bool install_service(const std::string& name, const std::string& exec_path,
                     const std::string& config_file, bool persist, const std::string& user) {
    std::string usr =
        user.empty() ? std::string(std::getenv("USER") ? std::getenv("USER") : "root") : user;
    return procutil::create_service_unit(name, exec_path, config_file, usr, persist);
}

bool uninstall_service(const std::string& name) { return procutil::remove_service_unit(name); }

ServiceStatus service_status(const std::string& name) {
    procutil::ServiceStatus st{};
    procutil::service_unit_status(name, st);
    return {st.exists, st.running};
}

bool start_service(const std::string& name) { return procutil::start_service_unit(name); }

bool stop_service(const std::string& name, bool force) {
    return procutil::stop_service_unit(name, force);
}

bool restart_service(const std::string& name, bool force) {
    return procutil::restart_service_unit(name, force);
}

bool start_daemon(const std::string& name) { return procutil::start_service_unit(name); }

bool stop_daemon(const std::string& name, bool force) {
    return procutil::stop_service_unit(name, force);
}

bool restart_daemon(const std::string& name, bool force) {
    return procutil::restart_service_unit(name, force);
}

} // namespace cli::platform
#endif
