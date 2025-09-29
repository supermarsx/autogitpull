#include "process_monitor.hpp"

#include <chrono>
#include <deque>
#include <thread>

#include "logger.hpp"
#include "ui_loop.hpp"

static int (*g_worker)(Options) = run_event_loop;

void set_monitor_worker(int (*func)(Options)) {
    if (func)
        g_worker = func;
    else
        g_worker = run_event_loop;
}

int run_with_monitor(const Options& opts) {
    if (!opts.service.persist)
        return g_worker(opts);
    std::deque<std::chrono::steady_clock::time_point> starts;
    int fail_count = 0;
    while (true) {
        auto now = std::chrono::steady_clock::now();
        starts.push_back(now);
        while (!starts.empty() && now - starts.front() > opts.service.respawn_window)
            starts.pop_front();
        if (opts.service.respawn_max > 0 &&
            static_cast<int>(starts.size()) > opts.service.respawn_max) {
            log_error("Respawn limit reached");
            break;
        }
        int rc = 0;
        try {
            rc = g_worker(opts);
        } catch (const std::exception& e) {
            log_error(std::string("Worker exception: ") + e.what());
            rc = 1;
        } catch (...) {
            log_error("Worker threw unknown exception");
            rc = 1;
        }
        log_info("Worker exited with code " + std::to_string(rc));
        auto delay = opts.service.respawn_delay * (1LL << fail_count);
        std::this_thread::sleep_for(delay);
        if (rc != 0)
            ++fail_count;
        else
            fail_count = 0;
    }
    return 0;
}
