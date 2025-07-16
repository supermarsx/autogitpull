#ifndef SYSTEM_UTILS_HPP
#define SYSTEM_UTILS_HPP
#include <cstdint>

namespace procutil {

bool set_cpu_affinity(unsigned long long mask);

} // namespace procutil

#endif // SYSTEM_UTILS_HPP
