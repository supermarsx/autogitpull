#ifndef LOCK_UTILS_HPP
#define LOCK_UTILS_HPP
#include <filesystem>

namespace procutil {

bool acquire_lock_file(const std::filesystem::path& path);
void release_lock_file(const std::filesystem::path& path);
bool read_lock_pid(const std::filesystem::path& path, unsigned long& pid);
bool process_running(unsigned long pid);
bool terminate_process(unsigned long pid);

struct LockFileGuard {
    std::filesystem::path path;
    bool locked = false;
    explicit LockFileGuard(const std::filesystem::path& p);
    ~LockFileGuard();
    LockFileGuard(const LockFileGuard&) = delete;
    LockFileGuard& operator=(const LockFileGuard&) = delete;
};

} // namespace procutil

#endif // LOCK_UTILS_HPP
