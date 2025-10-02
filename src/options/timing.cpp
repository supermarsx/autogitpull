// options_timing.cpp
//
// Parse timing intervals: main loop interval, UI refresh, and poll timers.

#include <map>
#include <string>
#include <climits>

#include "options.hpp"
#include "arg_parser.hpp"
#include "parse_utils.hpp"

void parse_timing_options(Options& opts, ArgParser& parser,
                          const std::function<std::string(const std::string&)>& cfg_opt,
                          const std::map<std::string, std::string>& cfg_opts) {
    bool ok = false;
    if (cfg_opts.count("--interval")) {
        auto dur = parse_duration(cfg_opt("--interval"), ok);
        if (!ok || dur.count() < 1 || dur.count() > INT_MAX)
            throw std::runtime_error("Invalid value for --interval");
        opts.interval = static_cast<int>(dur.count());
    }
    if (parser.has_flag("--interval")) {
        auto dur = parse_duration(parser, "--interval", ok);
        if (!ok || dur.count() < 1 || dur.count() > INT_MAX)
            throw std::runtime_error("Invalid value for --interval");
        opts.interval = static_cast<int>(dur.count());
    }
    if (cfg_opts.count("--refresh-rate")) {
        opts.refresh_ms = parse_time_ms(cfg_opt("--refresh-rate"), ok);
        if (!ok || opts.refresh_ms.count() < 1)
            throw std::runtime_error("Invalid value for --refresh-rate");
    }
    if (parser.has_flag("--refresh-rate")) {
        opts.refresh_ms = parse_time_ms(parser, "--refresh-rate", ok);
        if (!ok || opts.refresh_ms.count() < 1)
            throw std::runtime_error("Invalid value for --refresh-rate");
    }
    if (cfg_opts.count("--cpu-poll")) {
        auto dur = parse_duration(cfg_opt("--cpu-poll"), ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --cpu-poll");
        opts.limits.cpu_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (parser.has_flag("--cpu-poll")) {
        auto dur = parse_duration(parser, "--cpu-poll", ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --cpu-poll");
        opts.limits.cpu_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (cfg_opts.count("--mem-poll")) {
        auto dur = parse_duration(cfg_opt("--mem-poll"), ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --mem-poll");
        opts.limits.mem_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (parser.has_flag("--mem-poll")) {
        auto dur = parse_duration(parser, "--mem-poll", ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --mem-poll");
        opts.limits.mem_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (cfg_opts.count("--thread-poll")) {
        auto dur = parse_duration(cfg_opt("--thread-poll"), ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --thread-poll");
        opts.limits.thread_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (parser.has_flag("--thread-poll")) {
        auto dur = parse_duration(parser, "--thread-poll", ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --thread-poll");
        opts.limits.thread_poll_sec = static_cast<unsigned int>(dur.count());
    }
}
