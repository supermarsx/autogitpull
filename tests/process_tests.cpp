#include "test_common.hpp"

static int g_monitor_count = 0;

TEST_CASE("run_event_loop runtime limit") {
    fs::path dir = fs::temp_directory_path() / "runtime_limit_test";
    fs::create_directories(dir);
    Options opts;
    opts.root = dir;
    opts.cli = true;
    opts.interval = 1;
    opts.refresh_ms = std::chrono::milliseconds(100);
    opts.runtime_limit = std::chrono::seconds(2);
    auto start = std::chrono::steady_clock::now();
    int ret = run_event_loop(opts);
    auto elapsed = std::chrono::steady_clock::now() - start;
    fs::remove_all(dir);
    REQUIRE(ret == 0);
    REQUIRE(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 2);
    REQUIRE(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() < 5);
}

TEST_CASE("process monitor single run when disabled") {
    g_monitor_count = 0;
    auto worker_fn = [](const Options&) {
        ++g_monitor_count;
        return 0;
    };
    set_monitor_worker(worker_fn);
    Options opts;
    opts.persist = false;
    run_with_monitor(opts);
    REQUIRE(g_monitor_count == 1);
    set_monitor_worker(nullptr);
}

TEST_CASE("process monitor respawns up to limit") {
    g_monitor_count = 0;
    auto worker_fn = [](const Options&) {
        ++g_monitor_count;
        return 0;
    };
    set_monitor_worker(worker_fn);
    Options opts;
    opts.persist = true;
    opts.respawn_max = 2;
    opts.respawn_window = std::chrono::minutes(1);
    run_with_monitor(opts);
    REQUIRE(g_monitor_count == 2);
    set_monitor_worker(nullptr);
}
