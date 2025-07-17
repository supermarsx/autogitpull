#include "system_utils.hpp"
#include <sstream>
#ifdef __linux__
#include <sched.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace procutil {

bool set_cpu_affinity(unsigned long long mask) {
#ifdef __linux__
    if (mask == 0)
        return false;
    cpu_set_t set;
    CPU_ZERO(&set);
    for (unsigned i = 0; i < 64; ++i) {
        if (mask & (1ULL << i))
            CPU_SET(i, &set);
    }
    return sched_setaffinity(0, sizeof(set), &set) == 0;
#elif defined(_WIN32)
    if (mask == 0)
        return false;
    return SetProcessAffinityMask(GetCurrentProcess(), static_cast<DWORD_PTR>(mask)) != 0;
#else
    (void)mask;
    return false;
#endif
}

std::string get_cpu_affinity() {
#ifdef __linux__
    cpu_set_t set;
    if (sched_getaffinity(0, sizeof(set), &set) != 0)
        return "";
    std::ostringstream oss;
    bool first = true;
    for (int i = 0; i < CPU_SETSIZE; ++i) {
        if (CPU_ISSET(i, &set)) {
            if (!first)
                oss << ",";
            oss << i;
            first = false;
        }
    }
    return oss.str();
#elif defined(_WIN32)
    DWORD_PTR process_mask = 0, system_mask = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask))
        return "";
    std::ostringstream oss;
    bool first = true;
    for (int i = 0; i < static_cast<int>(sizeof(DWORD_PTR) * 8); ++i) {
        if (process_mask & (static_cast<DWORD_PTR>(1) << i)) {
            if (!first)
                oss << ",";
            oss << i;
            first = false;
        }
    }
    return oss.str();
#else
    return "";
#endif
}

} // namespace procutil
