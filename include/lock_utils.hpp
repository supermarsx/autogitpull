#ifndef LOCK_UTILS_HPP
#define LOCK_UTILS_HPP
#include <filesystem>

namespace lockfile {

class PidLock {
  public:
    explicit PidLock(const std::filesystem::path& dir);
    PidLock(const PidLock&) = delete;
    PidLock& operator=(const PidLock&) = delete;
    ~PidLock();

    bool acquired() const { return acquired_; }

  private:
    std::filesystem::path path_;
#ifdef _WIN32
    void* handle_;
#else
    int fd_;
#endif
    bool acquired_;
};

} // namespace lockfile

#endif // LOCK_UTILS_HPP
