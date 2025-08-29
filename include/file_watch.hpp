#ifndef FILE_WATCH_HPP
#define FILE_WATCH_HPP
#include <filesystem>
#include <functional>
#include <thread>
#include <atomic>

#if defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

/**
 * @brief Lightweight cross-platform file change watcher.
 *
 * Starts a background thread that invokes a callback whenever the watched
 * file is modified. The implementation uses inotify on Linux, FSEvents on
 * macOS and `ReadDirectoryChangesW` on Windows. For other platforms a
 * periodic polling fallback is used.
 */
class FileWatcher {
  public:
    FileWatcher(const std::filesystem::path& path, std::function<void()> callback);
    ~FileWatcher();

    /**
     * @brief Invoke the user-provided callback.
     *
     * Exposed for platform-specific hooks that cannot access private
     * members directly (e.g., macOS FSEvents callbacks).
     */
    void notify_change();

    /**
     * @brief Check if the watcher thread is active.
     */
    bool active() const;

    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

  private:
    std::filesystem::path path_;
    std::function<void()> callback_;
    std::atomic<bool> running_{false};
    std::thread thread_;
#if defined(__linux__)
    int inotify_fd_ = -1;
    int watch_desc_ = -1;
#elif defined(__APPLE__)
    FSEventStreamRef stream_ = nullptr;
#elif defined(_WIN32)
    HANDLE dir_handle_ = INVALID_HANDLE_VALUE;
    HANDLE change_handle_ = INVALID_HANDLE_VALUE;
#endif
};

#endif
