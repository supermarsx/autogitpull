#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include "test_common.hpp"

#include "file_watch.hpp"

namespace fs = std::filesystem;

#if defined(__linux__)
#include <dlfcn.h>
static bool g_fail_inotify = false;
extern "C" int inotify_init1(int flags) {
    if (g_fail_inotify)
        return -1;
    using func_t = int (*)(int);
    static func_t real_func = reinterpret_cast<func_t>(dlsym(RTLD_NEXT, "inotify_init1"));
    return real_func ? real_func(flags) : -1;
}
#endif

#if defined(__linux__)
#define WATCH_BACKEND "inotify"
#elif defined(__APPLE__)
#define WATCH_BACKEND "fsevents"
#elif defined(_WIN32)
#define WATCH_BACKEND "windows-api"
#else
#define WATCH_BACKEND "polling"
#endif

TEST_CASE("FileWatcher detects file modifications using " WATCH_BACKEND) {
    auto tmp = fs::temp_directory_path() / "watch_test.txt";
    {
        std::ofstream os(tmp);
        os << "initial";
    }
    std::atomic<int> hits{0};
    {
        FileWatcher watcher(tmp, [&]() { ++hits; });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        {
            std::ofstream os(tmp);
            os << "changed";
        }
        for (int i = 0; i < 20 && hits.load() == 0; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    FS_REMOVE(tmp);
    REQUIRE(hits.load() > 0);
}

#if defined(__linux__)
// When inotify_init1 fails the watcher should not start its background thread
// and the callback must never be invoked.
TEST_CASE("FileWatcher remains inactive when inotify_init1 fails") {
    auto tmp = fs::temp_directory_path() / "watch_fail.txt";
    {
        std::ofstream os(tmp);
        os << "initial";
    }
    std::atomic<int> hits{0};
    g_fail_inotify = true;
    {
        FileWatcher watcher(tmp, [&]() { ++hits; });
        REQUIRE_FALSE(watcher.active());
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        {
            std::ofstream os(tmp);
            os << "changed";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    g_fail_inotify = false;
    FS_REMOVE(tmp);
    REQUIRE(hits.load() == 0);
}
#endif
