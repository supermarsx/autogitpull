#include "file_watch.hpp"

#include <chrono>
#include <system_error>
#include <array>

#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#endif

using namespace std::chrono_literals;

FileWatcher::FileWatcher(const std::filesystem::path& path, std::function<void()> callback)
    : path_(path), callback_(std::move(callback)) {
    running_.store(true);
#ifdef __linux__
    inotify_fd_ = inotify_init1(IN_NONBLOCK);
    if (inotify_fd_ >= 0) {
        watch_desc_ =
            inotify_add_watch(inotify_fd_, path.c_str(), IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO);
    }
    thread_ = std::thread([this]() {
        std::array<char, 4096> buf{};
        while (running_) {
            int len = read(inotify_fd_, buf.data(), static_cast<int>(buf.size()));
            if (len > 0) {
                int i = 0;
                while (i < len) {
                    auto* ev = reinterpret_cast<inotify_event*>(buf.data() + i);
                    if (ev->wd == watch_desc_) {
                        callback_();
                    }
                    i += sizeof(inotify_event) + ev->len;
                }
            }
            std::this_thread::sleep_for(200ms);
        }
    });
#else
    auto prev = std::filesystem::last_write_time(path_);
    thread_ = std::thread([this, prev]() mutable {
        while (running_) {
            std::error_code ec;
            auto cur = std::filesystem::last_write_time(path_, ec);
            if (!ec && cur != prev) {
                prev = cur;
                callback_();
            }
            std::this_thread::sleep_for(500ms);
        }
    });
#endif
}

FileWatcher::~FileWatcher() {
    running_.store(false);
    if (thread_.joinable())
        thread_.join();
#ifdef __linux__
    if (watch_desc_ >= 0)
        inotify_rm_watch(inotify_fd_, watch_desc_);
    if (inotify_fd_ >= 0)
        close(inotify_fd_);
#endif
}
