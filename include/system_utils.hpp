#ifndef SYSTEM_UTILS_HPP
#define SYSTEM_UTILS_HPP
#include <cstdint>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace procutil {

bool set_cpu_affinity(unsigned long long mask);
/**
 * @brief Get a comma separated list of CPU cores the process is bound to.
 */
std::string get_cpu_affinity();

#ifdef _WIN32
class UniqueHandle {
  public:
    UniqueHandle() noexcept : h(INVALID_HANDLE_VALUE) {}
    explicit UniqueHandle(HANDLE handle) noexcept : h(handle) {}
    ~UniqueHandle() { reset(); }

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept : h(other.h) { other.h = INVALID_HANDLE_VALUE; }

    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset();
            h = other.h;
            other.h = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    HANDLE get() const noexcept { return h; }
    explicit operator bool() const noexcept { return h != nullptr && h != INVALID_HANDLE_VALUE; }

    HANDLE release() noexcept {
        HANDLE tmp = h;
        h = INVALID_HANDLE_VALUE;
        return tmp;
    }

    void reset(HANDLE handle = INVALID_HANDLE_VALUE) noexcept {
        if (h != nullptr && h != INVALID_HANDLE_VALUE)
            CloseHandle(h);
        h = handle;
    }

  private:
    HANDLE h;
};
#endif // _WIN32

class UniqueFd {
  public:
    UniqueFd() noexcept : fd(-1) {}
    explicit UniqueFd(int f) noexcept : fd(f) {}
    ~UniqueFd() { reset(); }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd(other.fd) { other.fd = -1; }

    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset();
            fd = other.fd;
            other.fd = -1;
        }
        return *this;
    }

    int get() const noexcept { return fd; }
    explicit operator bool() const noexcept { return fd >= 0; }

    int release() noexcept {
        int tmp = fd;
        fd = -1;
        return tmp;
    }

    void reset(int f = -1) noexcept {
        if (fd >= 0) {
#ifdef _WIN32
            _close(fd);
#else
            close(fd);
#endif
        }
        fd = f;
    }

  private:
    int fd;
};

} // namespace procutil

#endif // SYSTEM_UTILS_HPP
