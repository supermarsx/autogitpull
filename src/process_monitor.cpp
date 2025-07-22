#include "process_monitor.hpp"

#include <chrono>
#include <deque>
#include <thread>

#include "logger.hpp"
#include "ui_loop.hpp"

static int (*g_worker)(const Options&) = run_event_loop;

void set_monitor_worker(int (*func)(const Options&)) {
    if (func)
        g_worker = func;
    else
        g_worker = run_event_loop;
}

int run_with_monitor(const Options& opts) {
    if (!opts.persist)
        return g_worker(opts);
    std::deque<std::chrono::steady_clock::time_point> starts;
    while (true) {
        auto now = std::chrono::steady_clock::now();
        starts.push_back(now);
        while (!starts.empty() && now - starts.front() > opts.respawn_window)
            starts.pop_front();
        if (opts.respawn_max > 0 && static_cast<int>(starts.size()) > opts.respawn_max) {
            log_error("Respawn limit reached");
            break;
        }
        int rc = g_worker(opts);
        log_info("Worker exited with code " + std::to_string(rc));
        (void)rc;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
