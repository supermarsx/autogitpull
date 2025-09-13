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

/**
 * @brief Read accumulated user and system jiffies for this process.
 *
 * The Linux `/proc/self/stat` file exposes CPU time in so-called jiffies.
 * Parsing the 14th and 15th fields yields user and kernel time consumed by
 * the process. The value is cached by callers to avoid repeated file reads.
 *
 * @return Sum of user and system jiffies, or zero on failure.
 */
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

/**
 * @brief Extract a numeric entry from `/proc/self/status`.
 *
 * @param key Field name (including trailing colon) to search for.
 * @return Parsed value on success, or zero if the key is absent.
 */
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

/**
 * @brief Count threads by enumerating `/proc/self/task` entries.
 *
 * The directory contains one subdirectory per thread, providing an efficient
 * way to obtain the thread count without additional system calls.
 */
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
 * Querying `NtQuerySystemInformation(SystemProcessInformation, ...)` provides
 * a snapshot of all processes and their thread counts without creating a
 * snapshot handle. This is preferred over the slower Toolhelp32 interface.
 *
 * @return Number of threads in the current process or `0` on failure. The
 *         caller will fall back to Toolhelp32 iteration if this function
 *         returns `0`.
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

// Cached CPU time counters used to compute deltas between polls.
static long prev_jiffies = 0; ///< Last sampled jiffies on Linux.
#ifdef _WIN32
static ULONGLONG prev_proc_time = 0; ///< Last sampled process time on Windows.
#endif
#ifdef __APPLE__
static uint64_t prev_user = 0;   ///< Previous user time in microseconds.
static uint64_t prev_system = 0; ///< Previous system time in microseconds.
#endif

// Timestamps and intervals gate how often expensive system queries run.
static auto prev_time = std::chrono::steady_clock::now(); ///< Last CPU poll time.
static std::chrono::seconds cpu_poll_interval(5);         ///< Minimum gap between CPU polls.
static double last_cpu_percent = 0.0;                     ///< Cached CPU usage result.

static auto prev_mem_time = std::chrono::steady_clock::now(); ///< Last memory poll time.
static std::chrono::seconds mem_poll_interval(5);             ///< Minimum gap between memory polls.
static std::size_t last_mem_usage = 0;                        ///< Cached resident memory usage.

static auto prev_thread_time = std::chrono::steady_clock::now(); ///< Last thread count poll time.
static std::chrono::seconds thread_poll_interval(5); ///< Minimum gap between thread polls.
static std::size_t last_thread_count = 0;            ///< Cached thread count.
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

static std::size_t base_down = 0; ///< Baseline download counter.
static std::size_t base_up = 0;   ///< Baseline upload counter.

/**
 * @brief Configure how often CPU usage is recomputed.
 *
 * Polling too frequently would require repeatedly parsing `/proc` or calling
 * platform APIs. Values below one second are clamped to keep the load low.
 */
void set_cpu_poll_interval(unsigned int seconds) {
    cpu_poll_interval = std::chrono::seconds(seconds);
    if (cpu_poll_interval.count() < 1)
        cpu_poll_interval = std::chrono::seconds(1);
}

/**
 * @brief Configure how often resident memory usage is sampled.
 *
 * The interval prevents hitting `/proc` or equivalent APIs on every call.
 */
void set_memory_poll_interval(unsigned int seconds) {
    mem_poll_interval = std::chrono::seconds(seconds);
    if (mem_poll_interval.count() < 1)
        mem_poll_interval = std::chrono::seconds(1);
}

/**
 * @brief Configure how often thread counts are recomputed.
 */
void set_thread_poll_interval(unsigned int seconds) {
    thread_poll_interval = std::chrono::seconds(seconds);
    if (thread_poll_interval.count() < 1)
        thread_poll_interval = std::chrono::seconds(1);
}

/**
 * @brief Reset cached CPU statistics so the next poll starts fresh.
 */
void reset_cpu_usage() {
    prev_time = std::chrono::steady_clock::now();
#ifdef __linux__
    // Capture baseline jiffies from /proc for later delta computation.
    prev_jiffies = read_proc_jiffies();
#elif defined(_WIN32)
    // On Windows, GetProcessTimes yields 100-ns units of CPU time.
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
        // Convert to microseconds to align with std::chrono conversions.
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

/**
 * @brief Return the process CPU usage percentage.
 *
 * Results are cached for @c cpu_poll_interval to avoid hitting the operating
 * system on every call.
 */
double get_cpu_percent() {
#ifdef __linux__
    auto now = std::chrono::steady_clock::now();
    if (now - prev_time < cpu_poll_interval)
        return last_cpu_percent; // Respect polling interval.

    // Linux: compute usage based on jiffy deltas read from /proc/self/stat.
    long jiff = read_proc_jiffies();
    long diff_jiff = jiff - prev_jiffies;
    double diff_time = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time).count());
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
        return last_cpu_percent; // Respect polling interval.

    // Windows: GetProcessTimes returns 100-ns units; convert and normalize by core count.
    ULONGLONG proc = get_process_time();
    ULONGLONG diff_proc = proc - prev_proc_time;
    double diff_time = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time).count());
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
        return last_cpu_percent; // Respect polling interval.

    // macOS: query Mach task info for user/system times.
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
    double diff_time = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time).count());
    prev_time = now;
    if (diff_time <= 0)
        return last_cpu_percent;
    last_cpu_percent = 100.0 * static_cast<double>(diff_us) / diff_time;
    return last_cpu_percent;
#else
    // Other platforms: return cached value as no implementation is provided.
    return last_cpu_percent;
#endif
}

/**
 * @brief Return resident memory usage in megabytes.
 *
 * The value is cached for @c mem_poll_interval to avoid hitting the OS on every
 * call.
 */
std::size_t get_memory_usage_mb() {
    auto now = std::chrono::steady_clock::now();
    if (now - prev_mem_time < mem_poll_interval)
        return last_mem_usage; // Respect polling interval.
    prev_mem_time = now;
#ifdef __linux__
    // Parse VmRSS from /proc/self/status (value reported in kB).
    std::size_t rss_kb = read_status_value("VmRSS:");
    if (rss_kb > 0)
        last_mem_usage = rss_kb / 1024;
    else
        last_mem_usage = 0;
#elif defined(_WIN32)
    // Windows: use GetProcessMemoryInfo to get working set size.
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        last_mem_usage = static_cast<std::size_t>(pmc.WorkingSetSize / (1024 * 1024));
    else
        last_mem_usage = 0;
#elif defined(__APPLE__)
    // macOS: query Mach task info for resident size.
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

/**
 * @brief Return virtual memory usage in kilobytes.
 */
std::size_t get_virtual_memory_kb() {
#ifdef __linux__
    // Read page count from /proc/self/statm and multiply by page size.
    std::ifstream statm("/proc/self/statm");
    std::size_t pages = 0;
    if (!(statm >> pages))
        return 0;
    long page_size = sysconf(_SC_PAGESIZE);
    return pages * static_cast<std::size_t>(page_size) / 1024;
#elif defined(_WIN32)
    // Windows: PrivateUsage reports committed virtual memory in bytes.
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc)))
        return static_cast<std::size_t>(pmc.PrivateUsage / 1024);
    return 0;
#elif defined(__APPLE__)
    // macOS: use Mach task info to read virtual_size.
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

/**
 * @brief Return the number of threads in the current process.
 *
 * Thread counts are cached for @c thread_poll_interval to reduce system calls.
 */
std::size_t get_thread_count() {
    auto now = std::chrono::steady_clock::now();
    if (now - prev_thread_time < thread_poll_interval)
        return last_thread_count; // Respect polling interval.
    prev_thread_time = now;
#ifdef __linux__
    // Prefer fast /proc enumeration but fall back to parsing /proc/self/status.
    std::size_t count = count_task_threads();
    if (count == 0)
        count = read_status_value("Threads:");
    last_thread_count = count;
#elif defined(_WIN32)
    // Try the native NtQuerySystemInformation path first for efficiency.
    std::size_t count = query_thread_count_ntapi();
    if (count == 0) {
        // Fallback: enumerate threads with the Toolhelp32 snapshot API.
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
    // macOS: task_threads returns an array of thread ports.
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
/**
 * @brief Establish a baseline for network I/O counters.
 */
void init_network_usage() {
    auto bytes = read_net_bytes();
    base_down = bytes.first;
    base_up = bytes.second;
}

/**
 * @brief Return bytes sent/received since @ref init_network_usage.
 */
NetUsage get_network_usage() {
    auto bytes = read_net_bytes();
    NetUsage u;
    u.download_bytes = bytes.first - base_down;
    u.upload_bytes = bytes.second - base_up;
    return u;
}

/**
 * @brief Reset network usage counters to the current values.
 */
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

static std::size_t base_read = 0;        ///< Baseline disk read counter.
static std::size_t base_write = 0;       ///< Baseline disk write counter.
static std::uintmax_t base_dir_size = 0; ///< Baseline temp directory size.

/**
 * @brief Return the total size of regular files within a directory.
 */
static std::uintmax_t directory_size(const std::filesystem::path& p) {
    std::uintmax_t total = 0;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(p, ec)) {
        if (entry.is_regular_file(ec))
            total += entry.file_size(ec);
    }
    return total;
}

/**
 * @brief Establish a baseline for disk I/O and temporary directory size.
 */
void init_disk_usage() {
    auto b = read_io_bytes();
    base_read = b.first;
    base_write = b.second;
    base_dir_size = directory_size(std::filesystem::temp_directory_path());
}

/**
 * @brief Return disk bytes read/written since @ref init_disk_usage.
 */
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

/**
 * @brief Reset disk usage counters to the current values.
 */
void reset_disk_usage() { init_disk_usage(); }

} // namespace procutil
