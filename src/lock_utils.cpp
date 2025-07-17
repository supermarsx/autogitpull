#include "lock_utils.hpp"
#include <string>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace lockfile {

PidLock::PidLock(const std::filesystem::path& dir)
    : path_(dir / "autogitpull.pid"), acquired_(false)
#ifdef _WIN32
      ,
      handle_(nullptr)
#else
      ,
      fd_(-1)
#endif
{
#ifdef _WIN32
    handle_ = CreateFileW(path_.wstring().c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle_ != INVALID_HANDLE_VALUE) {
        DWORD pid = GetCurrentProcessId();
        char buf[32];
        DWORD written = 0;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%lu", static_cast<unsigned long>(pid));
        WriteFile(handle_, buf, static_cast<DWORD>(strlen(buf)), &written, NULL);
        acquired_ = true;
    } else {
        handle_ = INVALID_HANDLE_VALUE;
    }
#else
    fd_ = open(path_.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd_ >= 0) {
        std::string pid = std::to_string(getpid());
        write(fd_, pid.c_str(), pid.size());
        acquired_ = true;
    }
#endif
}

PidLock::~PidLock() {
    if (!acquired_)
        return;
#ifdef _WIN32
    if (handle_ != INVALID_HANDLE_VALUE)
        CloseHandle(handle_);
#else
    if (fd_ >= 0)
        close(fd_);
#endif
    std::error_code ec;
    std::filesystem::remove(path_, ec);
}

} // namespace lockfile
