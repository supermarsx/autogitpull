#pragma once

#include <optional>
#include <string>
#include <vector>

#include "options.hpp"

namespace cli {

/**
 * @brief Handle status-related CLI commands.
 *
 * Prints information about services, daemons, or running instances based on
 * user-specified flags. Returns an exit code when a status command is
 * processed or `std::nullopt` if no status flag was supplied.
 */
std::optional<int> handle_status_queries(const Options& opts);

/**
 * @brief Handle service management commands.
 *
 * Installs, uninstalls, or controls platform services according to @a opts.
 * Returns the resulting exit code when a service command is executed or
 * `std::nullopt` if no service action was requested.
 */
std::optional<int> handle_service_control(const Options& opts, const std::string& exec_path);

/**
 * @brief Handle daemon management commands.
 *
 * Installs, removes, or controls background daemons. Returns an exit code on
 * success or failure, or `std::nullopt` if no daemon operation was invoked.
 */
std::optional<int> handle_daemon_control(const Options& opts, const std::string& exec_path);

/**
 * @brief Handle destructive reset operations.
 *
 * Performs lock removal, process termination, or full data wipe depending on
 * the provided options. Returns an exit code when an action is taken or
 * `std::nullopt` if no reset option was specified.
 */
std::optional<int> handle_hard_reset(const Options& opts);

/**
 * @brief Execute the monitoring run loop.
 *
 * Validates alerting constraints and launches the process monitor when
 * permitted. Returns `0` on success and non-zero on failure.
 */
int handle_monitoring_run(const Options& opts);

namespace platform {
struct ServiceStatus {
    bool exists = false;
    bool running = false;
};

/**
 * @brief Derive the platform-specific service name from options.
 */
std::string service_name(const Options& opts);

/**
 * @brief Derive the platform-specific daemon name from options.
 */
std::string daemon_name(const Options& opts);

/**
 * @brief Check if the named service exists on the current platform.
 */
bool service_exists(const std::string& name);

/**
 * @brief List all services known to the platform.
 *
 * Each entry contains the service name and its existence/running state.
 */
std::vector<std::pair<std::string, ServiceStatus>> list_services();

/**
 * @brief Install a service using platform-specific mechanisms.
 *
 * @return `true` on success.
 */
bool install_service(const std::string& name, const std::string& exec_path,
                     const std::string& config_file, bool persist, const std::string& user);

/**
 * @brief Remove the named service from the platform.
 *
 * @return `true` if the service was successfully uninstalled.
 */
bool uninstall_service(const std::string& name);

/**
 * @brief Query the current status of a service.
 */
ServiceStatus service_status(const std::string& name);

/**
 * @brief Start a service via the platform's service manager.
 */
bool start_service(const std::string& name);

/**
 * @brief Stop a running service.
 *
 * @param force Attempt a forced stop when `true`.
 */
bool stop_service(const std::string& name, bool force);

/**
 * @brief Restart a service, optionally forcing termination.
 */
bool restart_service(const std::string& name, bool force);

/**
 * @brief Start a background daemon process.
 */
bool start_daemon(const std::string& name);

/**
 * @brief Stop a background daemon process.
 *
 * @param force Attempt a forced stop when `true`.
 */
bool stop_daemon(const std::string& name, bool force);

/**
 * @brief Restart a background daemon, optionally forcing termination.
 */
bool restart_daemon(const std::string& name, bool force);
} // namespace platform

} // namespace cli
