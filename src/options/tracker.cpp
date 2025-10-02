// options_tracker.cpp
//
// Tracker-related CLI parsing extracted from options.cpp to keep files small
// and focused, with clear documentation.

#include <functional>
#include <string>

#include "arg_parser.hpp"
#include "options.hpp"

/**
 * Parse tracker enable/disable flags from CLI and config.
 *
 * Supports CPU/memory/thread tracker negations and net tracker enable.
 */
void parse_tracker_options(Options& opts, ArgParser& parser,
                           const std::function<bool(const std::string&)>& cfg_flag) {
    struct TrackerFlag {
        const char* flag;
        bool negative;
        bool Options::*member;
    };
    const TrackerFlag trackers[] = {
        {"--no-cpu-tracker", true, &Options::cpu_tracker},
        {"--no-mem-tracker", true, &Options::mem_tracker},
        {"--no-thread-tracker", true, &Options::thread_tracker},
        {"--net-tracker", false, &Options::net_tracker},
    };
    for (const auto& t : trackers) {
        bool val = cfg_flag(t.flag);
        if (t.negative) {
            opts.*(t.member) = !val;
            if (parser.has_flag(t.flag))
                opts.*(t.member) = false;
        } else {
            opts.*(t.member) = val;
            if (parser.has_flag(t.flag))
                opts.*(t.member) = true;
        }
    }
}
