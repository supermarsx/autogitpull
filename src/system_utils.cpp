#include "system_utils.hpp"
#include <sstream>
#ifdef __linux__
#include <sched.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
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
#elif defined(__APPLE__)
    if (mask == 0)
        return false;
    unsigned core = 0;
    for (; core < 64; ++core) {
        if (mask & (1ULL << core))
            break;
    }
    if (core >= 64)
        return false;
    thread_affinity_policy_data_t policy;
    policy.affinity_tag = static_cast<integer_t>(core + 1);
    return thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY,
                             reinterpret_cast<thread_policy_t>(&policy), 1) == KERN_SUCCESS;
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
#elif defined(__APPLE__)
    task_affinity_tag_info_data_t info;
    mach_msg_type_number_t count = TASK_AFFINITY_TAG_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_AFFINITY_TAG_INFO, reinterpret_cast<task_info_t>(&info),
                  &count) != KERN_SUCCESS)
        return "";
    if (info.set_count == 0)
        return "";
    std::ostringstream oss;
    for (integer_t tag = info.min; tag <= info.max; ++tag) {
        if (tag != info.min)
            oss << ",";
        oss << (tag - 1);
    }
    return oss.str();
#else
    return "";
#endif
}

} // namespace procutil
