#include "lock_utils.hpp"
#include <fstream>
#include <system_error>
#include <cstring>
#include <set>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#ifdef __linux__
#include <sys/socket.h>
#include <sys/un.h>
#endif
#ifdef __APPLE__
#include <libproc.h>
#endif
#endif

namespace procutil {

bool acquire_lock_file(const std::filesystem::path& path) {
#ifdef _WIN32
    HANDLE h = CreateFileW(path.wstring().c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    DWORD pid = GetCurrentProcessId();
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%lu\n", static_cast<unsigned long>(pid));
    DWORD written = 0;
    WriteFile(h, buf, len, &written, NULL);
    CloseHandle(h);
    return true;
#else
    int fd = open(path.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1)
        return false;
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%ld\n", static_cast<long>(getpid()));
    write(fd, buf, len);
    close(fd);
    return true;
#endif
}

void release_lock_file(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    (void)ec;
}

bool read_lock_pid(const std::filesystem::path& path, unsigned long& pid) {
    std::ifstream f(path);
    if (!f.is_open())
        return false;
    f >> pid;
    return true;
}

bool process_running(unsigned long pid) {
#ifdef _WIN32
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (!h)
        return false;
    DWORD exit_code = 0;
    bool running = GetExitCodeProcess(h, &exit_code) && exit_code == STILL_ACTIVE;
    CloseHandle(h);
    return running;
#else
    if (kill(static_cast<pid_t>(pid), 0) == 0)
        return true;
    return errno != ESRCH;
#endif
}

bool terminate_process(unsigned long pid) {
#ifdef _WIN32
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (!h)
        return false;
    bool ok = TerminateProcess(h, 0);
    CloseHandle(h);
    return ok;
#else
    return kill(static_cast<pid_t>(pid), SIGTERM) == 0;
#endif
}

LockFileGuard::LockFileGuard(const std::filesystem::path& p) : path(p) {
    locked = acquire_lock_file(path);
}

LockFileGuard::~LockFileGuard() {
    if (locked)
        release_lock_file(path);
}

std::vector<std::pair<std::string, unsigned long>> find_running_instances() {
    namespace fs = std::filesystem;
    std::vector<std::pair<std::string, unsigned long>> out;
    auto add_inst = [&](const std::string& n, unsigned long p) { out.emplace_back(n, p); };

    fs::path tmp = fs::temp_directory_path();
    for (const auto& entry : fs::directory_iterator(tmp)) {
        if (entry.is_directory()) {
            fs::path lock = entry.path() / ".autogitpull.lock";
            unsigned long pid = 0;
            if (fs::exists(lock) && read_lock_pid(lock, pid) && process_running(pid))
                add_inst(entry.path().filename().string(), pid);
        }
#ifdef __linux__
        if (entry.path().extension() == ".sock") {
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd >= 0) {
                sockaddr_un addr{};
                addr.sun_family = AF_UNIX;
                std::strncpy(addr.sun_path, entry.path().c_str(), sizeof(addr.sun_path) - 1);
                if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
                    ucred cred{};
                    socklen_t len = sizeof(cred);
                    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) == 0)
                        add_inst(entry.path().stem().string(), cred.pid);
                }
                close(fd);
            }
        }
#endif
    }

#ifdef __linux__
    fs::path proc_dir("/proc");
    for (const auto& entry : fs::directory_iterator(proc_dir)) {
        if (!entry.is_directory())
            continue;
        std::string pidstr = entry.path().filename().string();
        if (!std::all_of(pidstr.begin(), pidstr.end(), ::isdigit))
            continue;
        unsigned long pid = 0;
        try {
            pid = std::stoul(pidstr);
        } catch (...) {
            continue;
        }
        std::ifstream f(entry.path() / "cmdline");
        if (!f.is_open())
            continue;
        std::string arg0;
        std::getline(f, arg0, '\0');
        if (fs::path(arg0).filename() == "autogitpull")
            add_inst("autogitpull", pid);
    }

#elif defined(_WIN32)
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        if (Process32First(snap, &pe)) {
            do {
#ifdef UNICODE
                std::wstring name = pe.szExeFile;
                if (name == L"autogitpull.exe" || name == L"autogitpull")
#else
                std::string name = pe.szExeFile;
                if (name == "autogitpull.exe" || name == "autogitpull")
#endif
                    add_inst("autogitpull", static_cast<unsigned long>(pe.th32ProcessID));
            } while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
    }

#elif defined(__APPLE__)
    int count = proc_listallpids(nullptr, 0);
    if (count > 0) {
        std::vector<pid_t> pids(static_cast<std::size_t>(count));
        count = proc_listallpids(pids.data(), static_cast<int>(pids.size() * sizeof(pid_t)));
        count /= static_cast<int>(sizeof(pid_t));
        char namebuf[PROC_PIDPATHINFO_MAXSIZE];
        for (int i = 0; i < count; ++i) {
            if (proc_name(pids[i], namebuf, sizeof(namebuf)) > 0) {
                if (std::string(namebuf) == "autogitpull")
                    add_inst("autogitpull", static_cast<unsigned long>(pids[i]));
            }
        }
    }
#endif

    return out;
}

} // namespace procutil
