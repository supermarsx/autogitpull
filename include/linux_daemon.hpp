#ifndef LINUX_DAEMON_HPP
#define LINUX_DAEMON_HPP

#include <string>

namespace procutil {

bool daemonize();

bool create_service_unit(const std::string& name, const std::string& exec_path,
                         const std::string& config_file, const std::string& user = "root");

bool remove_service_unit(const std::string& name);

} // namespace procutil

#endif // LINUX_DAEMON_HPP
