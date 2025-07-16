#include "resource_utils.hpp"
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#ifdef __linux__
#include <sched.h>
#include <unistd.h>
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

#else
static long read_proc_jiffies() { return 0; }
static std::size_t read_status_value(const std::string &) { return 0; }
#endif

static long prev_jiffies = 0;
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
#else
    return 0.0;
#endif
}

std::size_t get_memory_usage_mb() {
#ifdef __linux__
    std::size_t kb = read_status_value("VmRSS:");
    return kb / 1024;
#else
    return 0;
#endif
}

std::size_t get_thread_count() {
#ifdef __linux__
    return read_status_value("Threads:");
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
#else
    (void)cores;
    return false;
#endif
}

} // namespace procutil
