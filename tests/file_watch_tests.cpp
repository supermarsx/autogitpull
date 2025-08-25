#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "file_watch.hpp"

namespace fs = std::filesystem;

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
    fs::remove(tmp);
    REQUIRE(hits.load() > 0);
}
