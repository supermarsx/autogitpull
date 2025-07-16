#ifndef RESOURCE_UTILS_HPP
#define RESOURCE_UTILS_HPP
#include <cstddef>

namespace procutil {

double get_cpu_percent();
std::size_t get_memory_usage_mb();
std::size_t get_thread_count();
bool set_cpu_affinity(int cores);

struct NetUsage {
    std::size_t download_bytes;
    std::size_t upload_bytes;
};

void init_network_usage();
NetUsage get_network_usage();

} // namespace procutil

#endif // RESOURCE_UTILS_HPP
