#include "resource_utils.hpp"
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <sstream>
#include <utility>
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

static std::size_t read_status_value(const std::string& key) {
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
#ifdef __linux__
static std::pair<std::size_t, std::size_t> read_net_bytes() {
    std::ifstream net("/proc/self/net/dev");
    std::string line;
    std::getline(net, line);
    std::getline(net, line);
    std::size_t rx_total = 0, tx_total = 0;
    while (std::getline(net, line)) {
        std::istringstream iss(line);
        std::string iface;
        if (!std::getline(iss, iface, ':'))
            continue;
        std::size_t rx_bytes = 0, rx_packets = 0, rx_errs = 0, rx_drop = 0, rx_fifo = 0,
                    rx_frame = 0, rx_compressed = 0, rx_multicast = 0;
        std::size_t tx_bytes = 0, tx_packets = 0, tx_errs = 0, tx_drop = 0, tx_fifo = 0,
                    tx_colls = 0, tx_carrier = 0, tx_compressed = 0;
        iss >> rx_bytes >> rx_packets >> rx_errs >> rx_drop >> rx_fifo >> rx_frame >>
            rx_compressed >> rx_multicast >> tx_bytes >> tx_packets >> tx_errs >> tx_drop >>
            tx_fifo >> tx_colls >> tx_carrier >> tx_compressed;
        rx_total += rx_bytes;
        tx_total += tx_bytes;
    }
    return {rx_total, tx_total};
}
#else
static std::pair<std::size_t, std::size_t> read_net_bytes() { return {0, 0}; }
#endif

static std::size_t base_down = 0;
static std::size_t base_up = 0;

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

void init_network_usage() {
    auto bytes = read_net_bytes();
    base_down = bytes.first;
    base_up = bytes.second;
}

NetUsage get_network_usage() {
    auto bytes = read_net_bytes();
    NetUsage u;
    u.download_bytes = bytes.first - base_down;
    u.upload_bytes = bytes.second - base_up;
    return u;
}

} // namespace procutil
