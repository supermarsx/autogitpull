#ifndef RESOURCE_UTILS_HPP
#define RESOURCE_UTILS_HPP
#include <cstddef>

namespace procutil {

double get_cpu_percent();
std::size_t get_memory_usage_mb();
std::size_t get_thread_count();
bool set_cpu_affinity(int cores);

} // namespace procutil

#endif // RESOURCE_UTILS_HPP
