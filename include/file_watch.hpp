#ifndef FILE_WATCH_HPP
#define FILE_WATCH_HPP
#include <filesystem>
#include <functional>
#include <thread>
#include <atomic>

/**
 * @brief Lightweight cross-platform file change watcher.
 *
 * Starts a background thread that invokes a callback whenever the watched
 * file is modified. On Linux the implementation uses inotify; on other
 * platforms it falls back to periodic polling.
 */
class FileWatcher {
  public:
    FileWatcher(const std::filesystem::path& path, std::function<void()> callback);
    ~FileWatcher();

    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

  private:
    std::filesystem::path path_;
    std::function<void()> callback_;
    std::atomic<bool> running_{false};
    std::thread thread_;
#ifdef __linux__
    int inotify_fd_ = -1;
    int watch_desc_ = -1;
#endif
};

#endif
