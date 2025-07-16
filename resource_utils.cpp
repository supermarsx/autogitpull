#include "resource_utils.hpp"
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#ifdef __linux__
#include <sched.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif

namespace procutil {

#ifdef __linux__

static long read_proc_jiffies() {
    std::ifstream stat("/proc/self/stat");
    if (!stat)
        return 0;
    std::string tmp;
    for (int i = 0; i < 13; ++i)
        stat >> tmp; // skip fields
    long utime = 0, stime = 0;
    stat >> utime >> stime;
    return utime + stime;
}

static std::size_t read_status_value(const std::string &key) {
    std::ifstream status("/proc/self/status");
    std::string k;
    std::size_t val = 0;
    while (status >> k) {
        if (k == key) {
            status >> val;
            break;
        }
        std::string rest;
        std::getline(status, rest);
    }
    return val;
}

#elif defined(_WIN32)

static ULONGLONG get_process_time() {
    FILETIME ct, et, kt, ut;
    if (!GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut))
        return 0;
    ULARGE_INTEGER k, u;
    k.HighPart = kt.dwHighDateTime;
    k.LowPart = kt.dwLowDateTime;
    u.HighPart = ut.dwHighDateTime;
    u.LowPart = ut.dwLowDateTime;
    return k.QuadPart + u.QuadPart;
}

#else
static long read_proc_jiffies() { return 0; }
#endif

static long prev_jiffies = 0;
#ifdef _WIN32
static ULONGLONG prev_proc_time = 0;
#endif
static auto prev_time = std::chrono::steady_clock::now();

double get_cpu_percent() {
#ifdef __linux__
    long jiff = read_proc_jiffies();
    auto now = std::chrono::steady_clock::now();
    long diff_jiff = jiff - prev_jiffies;
    double diff_time =
        std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time).count();
    prev_jiffies = jiff;
    prev_time = now;
    if (diff_time <= 0)
        return 0.0;
    long hz = sysconf(_SC_CLK_TCK);
    double cpu_sec = static_cast<double>(diff_jiff) / hz;
    return 100.0 * cpu_sec / (diff_time / 1e6);
#elif defined(_WIN32)
    ULONGLONG proc = get_process_time();
    auto now = std::chrono::steady_clock::now();
    ULONGLONG diff_proc = proc - prev_proc_time;
    double diff_time =
        std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time).count();
    prev_proc_time = proc;
    prev_time = now;
    if (diff_time <= 0)
        return 0.0;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    double cpu_sec = static_cast<double>(diff_proc) / 1e7;
    return 100.0 * cpu_sec / (diff_time / 1e6) / si.dwNumberOfProcessors;
#else
    return 0.0;
#endif
}

std::size_t get_memory_usage_mb() {
#ifdef __linux__
    std::size_t kb = read_status_value("VmRSS:");
    return kb / 1024;
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return static_cast<std::size_t>(pmc.WorkingSetSize / (1024 * 1024));
    return 0;
#else
    return 0;
#endif
}

std::size_t get_thread_count() {
#ifdef __linux__
    return read_status_value("Threads:");
#elif defined(_WIN32)
    DWORD pid = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    THREADENTRY32 te;
    te.dwSize = sizeof(te);
    std::size_t count = 0;
    if (Thread32First(snap, &te)) {
        do {
            if (te.th32OwnerProcessID == pid)
                ++count;
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
    return count;
#else
    return 1;
#endif
}

bool set_cpu_affinity(int cores) {
#ifdef __linux__
    if (cores <= 0)
        return false;
    cpu_set_t set;
    CPU_ZERO(&set);
    for (int i = 0; i < cores; ++i)
        CPU_SET(i, &set);
    return sched_setaffinity(0, sizeof(set), &set) == 0;
#elif defined(_WIN32)
    if (cores <= 0)
        return false;
    DWORD_PTR mask = (static_cast<DWORD_PTR>(1) << cores) - 1;
    return SetProcessAffinityMask(GetCurrentProcess(), mask) != 0;
#else
    (void)cores;
    return false;
#endif
}

} // namespace procutil
