#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "file_watch.hpp"

namespace fs = std::filesystem;

TEST_CASE("FileWatcher detects file modifications") {
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
