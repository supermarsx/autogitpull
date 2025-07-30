#ifndef LINUX_DAEMON_HPP
#define LINUX_DAEMON_HPP

#include <string>
#include <vector>
#include <utility>

namespace procutil {

struct ServiceStatus {
    bool exists = false;
    bool running = false;
};

bool daemonize();

bool create_service_unit(const std::string& name, const std::string& exec_path,
                         const std::string& config_file, const std::string& user = "root",
                         bool persist = true);

bool remove_service_unit(const std::string& name);
bool service_unit_exists(const std::string& name);

bool start_service_unit(const std::string& name);
bool stop_service_unit(const std::string& name, bool force = false);
bool restart_service_unit(const std::string& name, bool force = false);
bool service_unit_status(const std::string& name, ServiceStatus& out);
std::vector<std::pair<std::string, ServiceStatus>> list_installed_services();

#ifndef _WIN32
int create_status_socket(const std::string& name);
int connect_status_socket(const std::string& name);
void remove_status_socket(const std::string& name, int fd);
#else
inline int create_status_socket(const std::string&) { return -1; }
inline int connect_status_socket(const std::string&) { return -1; }
inline void remove_status_socket(const std::string&, int) {}
inline std::vector<std::pair<std::string, ServiceStatus>> list_installed_services() { return {}; }
#endif

} // namespace procutil

#endif // LINUX_DAEMON_HPP
