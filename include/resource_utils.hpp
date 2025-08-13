#ifndef RESOURCE_UTILS_HPP
#define RESOURCE_UTILS_HPP
#include <cstddef>

namespace procutil {

/** @brief Return the approximate CPU usage of the current process. */
double get_cpu_percent();

/** @brief Reset cached CPU usage counters. */
void reset_cpu_usage();

/** @brief Set the polling interval for CPU usage in seconds. */
void set_cpu_poll_interval(unsigned int seconds);

/** @brief Set the polling interval for memory usage in seconds. */
void set_memory_poll_interval(unsigned int seconds);

/** @brief Set the polling interval for thread count in seconds. */
void set_thread_poll_interval(unsigned int seconds);

/** @brief Get the resident memory usage of the process in megabytes. */
std::size_t get_memory_usage_mb();

/** @brief Get the virtual memory usage of the process in kilobytes. */
std::size_t get_virtual_memory_kb();

/** @brief Retrieve the number of threads currently in the process. */
std::size_t get_thread_count();

struct NetUsage {
    std::size_t download_bytes;
    std::size_t upload_bytes;
};

/** @brief Record the current network usage as the baseline. */
void init_network_usage();

/** @brief Return the amount of bytes downloaded and uploaded since
 *         the call to @ref init_network_usage. */
NetUsage get_network_usage();

/** @brief Reset the network usage baseline. */
void reset_network_usage();

struct DiskUsage {
    std::size_t read_bytes;
    std::size_t write_bytes;
};

void init_disk_usage();
DiskUsage get_disk_usage();

/** @brief Reset the disk usage baseline. */
void reset_disk_usage();

} // namespace procutil

#endif // RESOURCE_UTILS_HPP
