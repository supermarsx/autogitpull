#include "lock_utils.hpp"
#include <fstream>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
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

LockFileGuard::LockFileGuard(const std::filesystem::path& p) : path(p) {
    locked = acquire_lock_file(path);
}

LockFileGuard::~LockFileGuard() {
    if (locked)
        release_lock_file(path);
}

} // namespace procutil
