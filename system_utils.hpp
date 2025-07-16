#ifndef SYSTEM_UTILS_HPP
#define SYSTEM_UTILS_HPP
#include <cstdint>
#include <string>

namespace procutil {

bool set_cpu_affinity(unsigned long long mask);
/**
 * @brief Get a comma separated list of CPU cores the process is bound to.
 */
std::string get_cpu_affinity();

} // namespace procutil

#endif // SYSTEM_UTILS_HPP
