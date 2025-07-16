#include "system_utils.hpp"
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

} // namespace procutil
