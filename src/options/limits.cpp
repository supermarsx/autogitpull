// options_limits.cpp
//
// Resource limits parsing separated from options.cpp to keep units 
// maintainable and within the size guidance.

#include <map>
#include <string>
#include <climits>

#include "arg_parser.hpp"
#include "options.hpp"
#include "parse_utils.hpp"

/**
 * Parse resource limit flags from CLI and config, writing to opts.limits.
 *
 * Handles CPU percent/cores, memory/download/upload/disk/traffic caps, and
 * max-depth. Throws std::runtime_error on invalid values.
 */
void parse_limits(Options& opts, ArgParser& parser,
                  const std::function<std::string(const std::string&)>& cfg_opt,
                  const std::map<std::string, std::string>& cfg_opts) {
    bool ok = false;
    if (cfg_opts.count("--cpu-percent")) {
        std::string v = cfg_opt("--cpu-percent");
        if (!v.empty() && v.back() == '%')
            v.pop_back();
        opts.limits.cpu_percent_limit = parse_double(v, 0.0, 100.0, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-percent");
    }
    if (parser.has_flag("--cpu-percent")) {
        std::string v = parser.get_option("--cpu-percent");
        if (!v.empty() && v.back() == '%')
            v.pop_back();
        opts.limits.cpu_percent_limit = parse_double(v, 0.0, 100.0, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-percent");
    }
    if (cfg_opts.count("--cpu-cores")) {
        opts.limits.cpu_core_mask = parse_ull(cfg_opt("--cpu-cores"), 0, ULLONG_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-cores");
    }
    if (parser.has_flag("--cpu-cores")) {
        opts.limits.cpu_core_mask = parse_ull(parser, "--cpu-cores", 0, ULLONG_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-cores");
    }

    struct ByteLimit {
        const char* flag;
        size_t ResourceLimits::*member;
        size_t divisor;
    };
    const ByteLimit limits[] = {
        {"--mem-limit", &ResourceLimits::mem_limit, 1024ull * 1024ull},
        {"--download-limit", &ResourceLimits::download_limit, 1024ull},
        {"--upload-limit", &ResourceLimits::upload_limit, 1024ull},
        {"--disk-limit", &ResourceLimits::disk_limit, 1024ull},
        {"--total-traffic-limit", &ResourceLimits::total_traffic_limit, 1},
    };
    for (const auto& lim : limits) {
        if (cfg_opts.count(lim.flag)) {
            size_t bytes = parse_bytes(cfg_opt(lim.flag), 0, SIZE_MAX, ok);
            if (!ok)
                throw std::runtime_error(std::string("Invalid value for ") + lim.flag);
            opts.limits.*(lim.member) = bytes / lim.divisor;
        }
        if (parser.has_flag(lim.flag)) {
            size_t bytes = parse_bytes(parser.get_option(lim.flag), 0, SIZE_MAX, ok);
            if (!ok)
                throw std::runtime_error(std::string("Invalid value for ") + lim.flag);
            opts.limits.*(lim.member) = bytes / lim.divisor;
        }
    }
    if (cfg_opts.count("--max-depth")) {
        opts.max_depth = parse_size_t(cfg_opt("--max-depth"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --max-depth");
    }
    if (parser.has_flag("--max-depth")) {
        opts.max_depth = parse_size_t(parser, "--max-depth", 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --max-depth");
    }
}
