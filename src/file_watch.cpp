#include "file_watch.hpp"

#include <chrono>
#include <system_error>
#include <array>
#include <cerrno>

#include "logger.hpp"

#if defined(__linux__)
#include <sys/inotify.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

using namespace std::chrono_literals;

namespace {
#if defined(__APPLE__)
void fsevent_cb(ConstFSEventStreamRef, void* user_data, size_t, void*,
                const FSEventStreamEventFlags*, const FSEventStreamEventId*) {
    auto* self = static_cast<FileWatcher*>(user_data);
    self->notify_change();
}
#endif
} // namespace

FileWatcher::FileWatcher(const std::filesystem::path& path, std::function<void()> callback)
    : path_(path), callback_(std::move(callback)) {
#if defined(__linux__)
    inotify_fd_ = inotify_init1(IN_NONBLOCK);
    if (inotify_fd_ >= 0) {
        watch_desc_ =
            inotify_add_watch(inotify_fd_, path.c_str(), IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO);
        if (watch_desc_ >= 0) {
            running_.store(true);
            thread_ = std::thread([this]() {
                std::array<char, 4096> buf{};
                while (running_) {
                    int len = read(inotify_fd_, buf.data(), static_cast<int>(buf.size()));
                    if (len > 0) {
                        int i = 0;
                        while (i < len) {
                            auto* ev = reinterpret_cast<inotify_event*>(buf.data() + i);
                            if (ev->wd == watch_desc_) {
                                notify_change();
                            }
                            i += sizeof(inotify_event) + ev->len;
                        }
                    }
                    std::this_thread::sleep_for(200ms);
                }
            });
        } else {
            std::error_code ec(errno, std::system_category());
            log_warning("inotify_add_watch failed; monitoring disabled: " + ec.message());
            close(inotify_fd_);
            inotify_fd_ = -1;
        }
    } else {
        std::error_code ec(errno, std::system_category());
        log_warning("inotify_init1 failed; monitoring disabled: " + ec.message());
    }
#elif defined(__APPLE__)
    running_.store(true);
    FSEventStreamContext ctx{};
    ctx.info = this;
    CFStringRef cpath = CFStringCreateWithCString(nullptr, path_.c_str(), kCFStringEncodingUTF8);
    CFArrayRef paths = CFArrayCreate(nullptr, (const void**)&cpath, 1, nullptr);
    stream_ = FSEventStreamCreate(nullptr, &fsevent_cb, &ctx, paths, kFSEventStreamEventIdSinceNow,
                                  0.0, kFSEventStreamCreateFlagFileEvents);
    CFRelease(cpath);
    CFRelease(paths);
    thread_ = std::thread([this]() {
        dispatch_queue_t queue =
            dispatch_queue_create("autogitpull.fsevents", DISPATCH_QUEUE_SERIAL);
        FSEventStreamSetDispatchQueue(stream_, queue);
        FSEventStreamStart(stream_);
        while (running_) {
            std::this_thread::sleep_for(500ms);
        }
        FSEventStreamStop(stream_);
        FSEventStreamInvalidate(stream_);
        dispatch_release(queue);
    });
#elif defined(_WIN32)
    running_.store(true);
    std::wstring dir = path_.parent_path().wstring();
    dir_handle_ = CreateFileW(dir.c_str(), FILE_LIST_DIRECTORY,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                              OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (dir_handle_ != INVALID_HANDLE_VALUE) {
        change_handle_ =
            FindFirstChangeNotificationW(dir.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    }
    thread_ = std::thread([this]() {
        while (running_) {
            DWORD status = WaitForSingleObject(change_handle_, 500);
            if (status == WAIT_OBJECT_0) {
                notify_change();
                FindNextChangeNotification(change_handle_);
            }
        }
    });
#else
    running_.store(true);
    auto prev = std::filesystem::last_write_time(path_);
    thread_ = std::thread([this, prev]() mutable {
        while (running_) {
            std::error_code ec;
            auto cur = std::filesystem::last_write_time(path_, ec);
            if (!ec && cur != prev) {
                prev = cur;
                notify_change();
            }
            std::this_thread::sleep_for(500ms);
        }
    });
#endif
}

void FileWatcher::notify_change() {
    if (callback_) {
        callback_();
    }
}

bool FileWatcher::active() const { return running_.load(); }

FileWatcher::~FileWatcher() {
    running_.store(false);
    if (thread_.joinable())
        thread_.join();
#if defined(__linux__)
    if (watch_desc_ >= 0)
        inotify_rm_watch(inotify_fd_, watch_desc_);
    if (inotify_fd_ >= 0)
        close(inotify_fd_);
#elif defined(__APPLE__)
    if (stream_)
        FSEventStreamRelease(stream_);
#elif defined(_WIN32)
    if (change_handle_ != INVALID_HANDLE_VALUE)
        FindCloseChangeNotification(change_handle_);
    if (dir_handle_ != INVALID_HANDLE_VALUE)
        CloseHandle(dir_handle_);
#endif
}
