#include "resource_utils.hpp"
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <sstream>
#include <utility>
#include <filesystem>
#include <system_error>
#include <cstdint>
#ifdef __linux__
#include <sched.h>
#include <unistd.h>
#include <sys/resource.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winternl.h>
#include <vector>
#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)
#endif

#elif defined(__APPLE__)
#include <mach/mach.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif
#include "system_utils.hpp"

namespace procutil {

#ifdef __linux__

static long read_proc_jiffies() {
    std::ifstream stat("/proc/self/stat");
    if (!stat.is_open())
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

static std::size_t count_task_threads() {
    namespace fs = std::filesystem;
    try {
        return static_cast<std::size_t>(
            std::distance(fs::directory_iterator("/proc/self/task"), fs::directory_iterator()));
    } catch (...) {
        return 0;
    }
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

/**
 * @brief Try to obtain the thread count using the NtQuerySystemInformation API.
 *
 * Falls back to Toolhelp32 if the call fails.
 */
static std::size_t query_thread_count_ntapi() {
    using NtQuerySystemInformationPtr =
        NTSTATUS(WINAPI*)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
    auto nt = reinterpret_cast<NtQuerySystemInformationPtr>(
        GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation"));
    if (!nt)
        return 0;
    ULONG len = 0;
    NTSTATUS status = nt(SystemProcessInformation, nullptr, 0, &len);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
        return 0;
    std::vector<char> buf(len);
    if (nt(SystemProcessInformation, buf.data(), len, &len) != 0)
        return 0;
    DWORD pid = GetCurrentProcessId();
    auto* p = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(buf.data());
    while (true) {
        if ((DWORD_PTR)p->UniqueProcessId == pid)
            return p->NumberOfThreads;
        if (p->NextEntryOffset == 0)
            break;
        p = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(reinterpret_cast<char*>(p) +
                                                          p->NextEntryOffset);
    }
    return 0;
}

#else
static long read_proc_jiffies() { return 0; }
#endif

static long prev_jiffies = 0;
#ifdef _WIN32
static ULONGLONG prev_proc_time = 0;
#endif
#ifdef __APPLE__
static uint64_t prev_user = 0;
static uint64_t prev_system = 0;
#endif
static auto prev_time = std::chrono::steady_clock::now();
static std::chrono::seconds cpu_poll_interval(5);
static double last_cpu_percent = 0.0;
static auto prev_mem_time = std::chrono::steady_clock::now();
static std::chrono::seconds mem_poll_interval(5);
static std::size_t last_mem_usage = 0;
static auto prev_thread_time = std::chrono::steady_clock::now();
static std::chrono::seconds thread_poll_interval(5);
static std::size_t last_thread_count = 0;
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
#elif defined(__APPLE__)
static std::pair<std::size_t, std::size_t> read_net_bytes() {
    ifaddrs* ifap = nullptr;
    if (getifaddrs(&ifap) != 0)
        return {0, 0};
    std::size_t rx_total = 0, tx_total = 0;
    for (ifaddrs* ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_data)
            continue;
        auto* data = reinterpret_cast<if_data*>(ifa->ifa_data);
        rx_total += data->ifi_ibytes;
        tx_total += data->ifi_obytes;
    }
    freeifaddrs(ifap);
    return {rx_total, tx_total};
}
#else
static std::pair<std::size_t, std::size_t> read_net_bytes() { return {0, 0}; }
#endif

static std::size_t base_down = 0;
static std::size_t base_up = 0;

void set_cpu_poll_interval(unsigned int seconds) {
    cpu_poll_interval = std::chrono::seconds(seconds);
    if (cpu_poll_interval.count() < 1)
        cpu_poll_interval = std::chrono::seconds(1);
}

void set_memory_poll_interval(unsigned int seconds) {
    mem_poll_interval = std::chrono::seconds(seconds);
    if (mem_poll_interval.count() < 1)
        mem_poll_interval = std::chrono::seconds(1);
}

void set_thread_poll_interval(unsigned int seconds) {
    thread_poll_interval = std::chrono::seconds(seconds);
    if (thread_poll_interval.count() < 1)
        thread_poll_interval = std::chrono::seconds(1);
}

void reset_cpu_usage() {
    prev_time = std::chrono::steady_clock::now();
#ifdef __linux__
    prev_jiffies = read_proc_jiffies();
#elif defined(_WIN32)
    prev_proc_time = get_process_time();
#elif defined(__APPLE__)
    mach_msg_type_number_t count = 0;
    task_flavor_t flavor = MACH_TASK_BASIC_INFO;
#ifdef TASK_BASIC_INFO_64
    task_basic_info_64_data_t info;
    flavor = TASK_BASIC_INFO_64;
    count = TASK_BASIC_INFO_64_COUNT;
#else
    mach_task_basic_info_data_t info;
    count = MACH_TASK_BASIC_INFO_COUNT;
#endif
    if (task_info(mach_task_self(), flavor, reinterpret_cast<task_info_t>(&info), &count) ==
        KERN_SUCCESS) {
        prev_user = static_cast<uint64_t>(info.user_time.seconds) * 1000000ULL +
                    info.user_time.microseconds;
        prev_system = static_cast<uint64_t>(info.system_time.seconds) * 1000000ULL +
                      info.system_time.microseconds;
    } else {
        prev_user = 0;
        prev_system = 0;
    }
#endif
    last_cpu_percent = 0.0;
}

double get_cpu_percent() {
#ifdef __linux__
    auto now = std::chrono::steady_clock::now();
    if (now - prev_time < cpu_poll_interval)
        return last_cpu_percent;
    long jiff = read_proc_jiffies();
    long diff_jiff = jiff - prev_jiffies;
    double diff_time =
        std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time).count();
    prev_jiffies = jiff;
    prev_time = now;
    if (diff_time <= 0)
        return last_cpu_percent;
    long hz = sysconf(_SC_CLK_TCK);
    double cpu_sec = static_cast<double>(diff_jiff) / hz;
    last_cpu_percent = 100.0 * cpu_sec / (diff_time / 1e6);
    return last_cpu_percent;
#elif defined(_WIN32)
    auto now = std::chrono::steady_clock::now();
    if (now - prev_time < cpu_poll_interval)
        return last_cpu_percent;
    ULONGLONG proc = get_process_time();
    ULONGLONG diff_proc = proc - prev_proc_time;
    double diff_time =
        std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time).count();
    prev_proc_time = proc;
    prev_time = now;
    if (diff_time <= 0)
        return last_cpu_percent;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    double cpu_sec = static_cast<double>(diff_proc) / 1e7;
    last_cpu_percent = 100.0 * cpu_sec / (diff_time / 1e6) / si.dwNumberOfProcessors;
    return last_cpu_percent;
#elif defined(__APPLE__)
    auto now = std::chrono::steady_clock::now();
    if (now - prev_time < cpu_poll_interval)
        return last_cpu_percent;
    mach_msg_type_number_t count = 0;
    task_flavor_t flavor = MACH_TASK_BASIC_INFO;
#ifdef TASK_BASIC_INFO_64
    task_basic_info_64_data_t info;
    flavor = TASK_BASIC_INFO_64;
    count = TASK_BASIC_INFO_64_COUNT;
#else
    mach_task_basic_info_data_t info;
    count = MACH_TASK_BASIC_INFO_COUNT;
#endif
    if (task_info(mach_task_self(), flavor, reinterpret_cast<task_info_t>(&info), &count) !=
        KERN_SUCCESS)
        return last_cpu_percent;
    uint64_t user_us =
        static_cast<uint64_t>(info.user_time.seconds) * 1000000ULL + info.user_time.microseconds;
    uint64_t system_us = static_cast<uint64_t>(info.system_time.seconds) * 1000000ULL +
                         info.system_time.microseconds;
    uint64_t diff_us = (user_us - prev_user) + (system_us - prev_system);
    prev_user = user_us;
    prev_system = system_us;
    double diff_time =
        std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time).count();
    prev_time = now;
    if (diff_time <= 0)
        return last_cpu_percent;
    last_cpu_percent = 100.0 * static_cast<double>(diff_us) / diff_time;
    return last_cpu_percent;
#else
    return last_cpu_percent;
#endif
}

std::size_t get_memory_usage_mb() {
    auto now = std::chrono::steady_clock::now();
    if (now - prev_mem_time < mem_poll_interval)
        return last_mem_usage;
    prev_mem_time = now;
#ifdef __linux__
    std::size_t rss_kb = read_status_value("VmRSS:");
    if (rss_kb > 0)
        last_mem_usage = rss_kb / 1024;
    else
        last_mem_usage = 0;
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        last_mem_usage = static_cast<std::size_t>(pmc.WorkingSetSize / (1024 * 1024));
    else
        last_mem_usage = 0;
#elif defined(__APPLE__)
    mach_msg_type_number_t count = 0;
    task_flavor_t flavor = MACH_TASK_BASIC_INFO;
#ifdef TASK_BASIC_INFO_64
    task_basic_info_64_data_t info;
    flavor = TASK_BASIC_INFO_64;
    count = TASK_BASIC_INFO_64_COUNT;
#else
    mach_task_basic_info_data_t info;
    count = MACH_TASK_BASIC_INFO_COUNT;
#endif
    if (task_info(mach_task_self(), flavor, reinterpret_cast<task_info_t>(&info), &count) ==
        KERN_SUCCESS)
        last_mem_usage = static_cast<std::size_t>(info.resident_size / (1024 * 1024));
    else
        last_mem_usage = 0;
#else
    last_mem_usage = 0;
#endif
    return last_mem_usage;
}

std::size_t get_virtual_memory_kb() {
#ifdef __linux__
    std::ifstream statm("/proc/self/statm");
    std::size_t pages = 0;
    if (!(statm >> pages))
        return 0;
    long page_size = sysconf(_SC_PAGESIZE);
    return pages * static_cast<std::size_t>(page_size) / 1024;
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc)))
        return static_cast<std::size_t>(pmc.PrivateUsage / 1024);
    return 0;
#elif defined(__APPLE__)
    mach_msg_type_number_t count = 0;
    task_flavor_t flavor = MACH_TASK_BASIC_INFO;
#ifdef TASK_BASIC_INFO_64
    task_basic_info_64_data_t info;
    flavor = TASK_BASIC_INFO_64;
    count = TASK_BASIC_INFO_64_COUNT;
#else
    mach_task_basic_info_data_t info;
    count = MACH_TASK_BASIC_INFO_COUNT;
#endif
    if (task_info(mach_task_self(), flavor, reinterpret_cast<task_info_t>(&info), &count) ==
        KERN_SUCCESS)
        return static_cast<std::size_t>(info.virtual_size / 1024);
    return 0;
#else
    return 0;
#endif
}

std::size_t get_thread_count() {
    auto now = std::chrono::steady_clock::now();
    if (now - prev_thread_time < thread_poll_interval)
        return last_thread_count;
    prev_thread_time = now;
#ifdef __linux__
    std::size_t count = count_task_threads();
    if (count == 0)
        count = read_status_value("Threads:");
    last_thread_count = count;
#elif defined(_WIN32)
    std::size_t count = query_thread_count_ntapi();
    if (count == 0) {
        DWORD pid = GetCurrentProcessId();
        UniqueHandle snap(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0));
        if (!snap) {
            last_thread_count = 0;
        } else {
            THREADENTRY32 te;
            te.dwSize = sizeof(te);
            count = 0;
            if (Thread32First(snap.get(), &te)) {
                do {
                    if (te.th32OwnerProcessID == pid)
                        ++count;
                } while (Thread32Next(snap.get(), &te));
            }
            last_thread_count = count;
        }
    } else {
        last_thread_count = count;
    }
#elif defined(__APPLE__)
    thread_act_array_t threads;
    mach_msg_type_number_t count;
    if (task_threads(mach_task_self(), &threads, &count) != KERN_SUCCESS) {
        last_thread_count = 0;
    } else {
        vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(threads),
                      count * sizeof(thread_act_t));
        last_thread_count = static_cast<std::size_t>(count);
    }
#else
    last_thread_count = 1;
#endif
    return last_thread_count;
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

void reset_network_usage() { init_network_usage(); }

#ifdef __linux__
static std::pair<std::size_t, std::size_t> read_io_bytes() {
    std::ifstream io("/proc/self/io");
    std::string key;
    std::size_t read_b = 0, write_b = 0;
    while (io >> key) {
        if (key == "read_bytes:") {
            io >> read_b;
        } else if (key == "write_bytes:") {
            io >> write_b;
        } else {
            std::string rest;
            std::getline(io, rest);
        }
    }
    return {read_b, write_b};
}
#elif defined(_WIN32)
static std::pair<std::size_t, std::size_t> read_io_bytes() {
    IO_COUNTERS counters;
    if (GetProcessIoCounters(GetCurrentProcess(), &counters))
        return {static_cast<std::size_t>(counters.ReadTransferCount),
                static_cast<std::size_t>(counters.WriteTransferCount)};
    return {0, 0};
}
#elif defined(__APPLE__)
#include <libproc.h>
static std::pair<std::size_t, std::size_t> read_io_bytes() {
    rusage_info_v2 info;
    if (proc_pid_rusage(getpid(), RUSAGE_INFO_V2, reinterpret_cast<rusage_info_t*>(&info)) == 0)
        return {static_cast<std::size_t>(info.ri_diskio_bytesread),
                static_cast<std::size_t>(info.ri_diskio_byteswritten)};
    return {0, 0};
}
#else
static std::pair<std::size_t, std::size_t> read_io_bytes() {
    std::ifstream ifs("/proc/self/io");
    if (ifs.is_open()) {
        std::string key;
        std::size_t value;
        std::size_t read = 0;
        std::size_t write = 0;
        while (ifs >> key >> value) {
            if (key == "read_bytes:")
                read = value;
            else if (key == "write_bytes:")
                write = value;
        }
        return {read, write};
    }
    rusage ru{};
    if (getrusage(RUSAGE_SELF, &ru) == 0)
        return {static_cast<std::size_t>(ru.ru_inblock) * 512,
                static_cast<std::size_t>(ru.ru_oublock) * 512};
    return {0, 0};
}
#endif

static std::size_t base_read = 0;
static std::size_t base_write = 0;
static std::uintmax_t base_dir_size = 0;

static std::uintmax_t directory_size(const std::filesystem::path& p) {
    std::uintmax_t total = 0;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(p, ec)) {
        if (entry.is_regular_file(ec))
            total += entry.file_size(ec);
    }
    return total;
}

void init_disk_usage() {
    auto b = read_io_bytes();
    base_read = b.first;
    base_write = b.second;
    base_dir_size = directory_size(std::filesystem::temp_directory_path());
}

DiskUsage get_disk_usage() {
    auto b = read_io_bytes();
    DiskUsage u;
    u.read_bytes = b.first - base_read;
    u.write_bytes = b.second - base_write;
    if (u.read_bytes == 0 && u.write_bytes == 0) {
        auto current = directory_size(std::filesystem::temp_directory_path());
        u.write_bytes =
            current > base_dir_size ? static_cast<std::size_t>(current - base_dir_size) : 0;
    }
    return u;
}

void reset_disk_usage() { init_disk_usage(); }

} // namespace procutil
