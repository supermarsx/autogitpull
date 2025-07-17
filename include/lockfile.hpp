#ifndef LOCKFILE_HPP
#define LOCKFILE_HPP
#include <filesystem>
#include <string>

/** RAII class that creates a lock directory inside the target
 *  repository root to prevent multiple instances from running. */
class LockFile {
  public:
    explicit LockFile(const std::filesystem::path& dir);
    ~LockFile();
    bool acquired() const;
    const std::string& error() const;

  private:
    std::filesystem::path lock_dir_;
    bool locked_ = false;
    std::string err_;
};

#endif // LOCKFILE_HPP
