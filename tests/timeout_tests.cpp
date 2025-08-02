#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <chrono>
#include <thread>

#include "ui_loop.hpp"
#include "thread_compat.hpp"

TEST_CASE("poll_timed_out terminates worker") {
    Options opts;
    opts.exit_on_timeout = true;
    opts.pull_timeout = std::chrono::seconds(1);
    auto start = std::chrono::steady_clock::now();
    std::atomic<bool> running{true};

    th_compat::jthread worker([&]() {
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    bool timed = poll_timed_out(opts, start, std::chrono::steady_clock::now());
    REQUIRE(timed);
    running.store(false);
#if defined(__cpp_lib_jthread)
    worker.request_stop();
#endif
    worker.join();
    REQUIRE_FALSE(worker.joinable());
}
