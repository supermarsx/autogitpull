#pragma once

#include <optional>
#include <string>
#include <vector>

#include "options.hpp"

namespace cli {

std::optional<int> handle_status_queries(const Options& opts);
std::optional<int> handle_service_control(const Options& opts, const std::string& exec_path);
std::optional<int> handle_daemon_control(const Options& opts, const std::string& exec_path);
std::optional<int> handle_hard_reset(const Options& opts);
int handle_monitoring_run(const Options& opts);

namespace platform {
struct ServiceStatus {
    bool exists = false;
    bool running = false;
};

std::string service_name(const Options& opts);
std::string daemon_name(const Options& opts);

bool service_exists(const std::string& name);
std::vector<std::pair<std::string, ServiceStatus>> list_services();
bool install_service(const std::string& name, const std::string& exec_path,
                     const std::string& config_file, bool persist, const std::string& user);
bool uninstall_service(const std::string& name);
ServiceStatus service_status(const std::string& name);
bool start_service(const std::string& name);
bool stop_service(const std::string& name, bool force);
bool restart_service(const std::string& name, bool force);
bool start_daemon(const std::string& name);
bool stop_daemon(const std::string& name, bool force);
bool restart_daemon(const std::string& name, bool force);
} // namespace platform

} // namespace cli
