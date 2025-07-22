#include <algorithm>
#include <chrono>
#include <cctype>
#include <climits>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "arg_parser.hpp"
#include "config_utils.hpp"
#include "options.hpp"
#include "parse_utils.hpp"

namespace fs = std::filesystem;

Options parse_options(int argc, char* argv[]) {
    // Parse config file option first
    const std::set<std::string> pre_known{"--config-yaml", "--config-json"};
    const std::map<char, std::string> pre_short{{'y', "--config-yaml"}, {'j', "--config-json"}};
    ArgParser pre_parser(argc, argv, pre_known, pre_short);
    std::map<std::string, std::string> cfg_opts;
    if (pre_parser.has_flag("--config-yaml")) {
        std::string cfg = pre_parser.get_option("--config-yaml");
        if (cfg.empty())
            throw std::runtime_error("--config-yaml requires a file");
        std::string err;
        if (!load_yaml_config(cfg, cfg_opts, err))
            throw std::runtime_error("Failed to load config: " + err);
    }
    if (pre_parser.has_flag("--config-json")) {
        std::string cfg = pre_parser.get_option("--config-json");
        if (cfg.empty())
            throw std::runtime_error("--config-json requires a file");
        std::string err;
        if (!load_json_config(cfg, cfg_opts, err))
            throw std::runtime_error("Failed to load config: " + err);
    }

    const std::set<std::string> known{
        "--include-private",  "--show-skipped",   "--show-version",      "--version",
        "--interval",         "--refresh-rate",   "--cpu-poll",          "--mem-poll",
        "--thread-poll",      "--log-dir",        "--log-file",          "--concurrency",
        "--check-only",       "--no-hash-check",  "--log-level",         "--verbose",
        "--max-threads",      "--cpu-percent",    "--cpu-cores",         "--mem-limit",
        "--no-cpu-tracker",   "--no-mem-tracker", "--no-thread-tracker", "--help",
        "--threads",          "--single-thread",  "--net-tracker",       "--download-limit",
        "--upload-limit",     "--disk-limit",     "--max-depth",         "--cli",
        "--single-run",       "--silent",         "--recursive",         "--config-yaml",
        "--config-json",      "--ignore",         "--force-pull",        "--discard-dirty",
        "--debug-memory",     "--dump-state",     "--dump-large",        "--install-daemon",
        "--uninstall-daemon", "--daemon-config",  "--install-service",   "--uninstall-service",
        "--service-config",   "--attach",         "--background",        "--reattach",
        "--remove-lock",      "--show-runtime",   "--max-runtime"};
    const std::map<char, std::string> short_opts{
        {'p', "--include-private"}, {'k', "--show-skipped"}, {'v', "--show-version"},
        {'V', "--version"},         {'i', "--interval"},     {'r', "--refresh-rate"},
        {'d', "--log-dir"},         {'l', "--log-file"},     {'y', "--config-yaml"},
        {'j', "--config-json"},     {'c', "--cli"},          {'s', "--silent"},
        {'D', "--max-depth"},       {'h', "--help"},         {'A', "--attach"},
        {'b', "--background"},      {'B', "--reattach"},     {'R', "--remove-lock"}};
    ArgParser parser(argc, argv, known, short_opts);

    auto cfg_flag = [&](const std::string& k) {
        auto it = cfg_opts.find(k);
        if (it == cfg_opts.end())
            return false;
        std::string v = it->second;
        std::transform(v.begin(), v.end(), v.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return v == "" || v == "1" || v == "true" || v == "yes";
    };
    auto cfg_opt = [&](const std::string& k) {
        auto it = cfg_opts.find(k);
        if (it != cfg_opts.end())
            return it->second;
        return std::string();
    };

    Options opts;
    opts.cli = parser.has_flag("--cli") || cfg_flag("--cli");
    opts.single_run = parser.has_flag("--single-run") || cfg_flag("--single-run");
    if (opts.single_run)
        opts.cli = true;
    opts.install_daemon = parser.has_flag("--install-daemon") || cfg_flag("--install-daemon");
    opts.uninstall_daemon = parser.has_flag("--uninstall-daemon") || cfg_flag("--uninstall-daemon");
    if (parser.has_flag("--daemon-config") || cfg_opts.count("--daemon-config")) {
        std::string val = parser.get_option("--daemon-config");
        if (val.empty())
            val = cfg_opt("--daemon-config");
        opts.daemon_config = val;
    }
    opts.install_service = parser.has_flag("--install-service") || cfg_flag("--install-service");
    opts.uninstall_service =
        parser.has_flag("--uninstall-service") || cfg_flag("--uninstall-service");
    if (parser.has_flag("--service-config") || cfg_opts.count("--service-config")) {
        std::string val = parser.get_option("--service-config");
        if (val.empty())
            val = cfg_opt("--service-config");
        opts.service_config = val;
    }
    if (parser.has_flag("--attach") || cfg_opts.count("--attach")) {
        std::string val = parser.get_option("--attach");
        if (val.empty())
            val = cfg_opt("--attach");
        if (val.empty())
            throw std::runtime_error("--attach requires a name");
        opts.attach_name = val;
    }
    opts.run_background = parser.has_flag("--background") || cfg_opts.count("--background");
    if (opts.run_background) {
        std::string val = parser.get_option("--background");
        if (val.empty())
            val = cfg_opt("--background");
        if (val.empty())
            throw std::runtime_error("--background requires a name");
        opts.attach_name = val;
    }
    if (parser.has_flag("--reattach") || cfg_opts.count("--reattach")) {
        std::string val = parser.get_option("--reattach");
        if (val.empty())
            val = cfg_opt("--reattach");
        if (val.empty())
            throw std::runtime_error("--reattach requires a name");
        opts.attach_name = val;
        opts.reattach = true;
    }
    opts.show_runtime = parser.has_flag("--show-runtime") || cfg_flag("--show-runtime");
    if (parser.has_flag("--max-runtime") || cfg_opts.count("--max-runtime")) {
        std::string val = parser.get_option("--max-runtime");
        if (val.empty())
            val = cfg_opt("--max-runtime");
        bool ok = false;
        int sec = parse_int(val, 1, INT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --max-runtime");
        opts.runtime_limit = std::chrono::seconds(sec);
    }
    opts.silent = parser.has_flag("--silent") || cfg_flag("--silent");
    opts.recursive_scan = parser.has_flag("--recursive") || cfg_flag("--recursive");
    opts.show_help = parser.has_flag("--help");
    opts.print_version = parser.has_flag("--version");
    if (parser.positional().size() != 1 && !opts.show_help && !opts.print_version &&
        ((opts.attach_name.empty() && !opts.reattach) || opts.run_background))
        throw std::runtime_error("Root path required");
    if (!parser.unknown_flags().empty()) {
        throw std::runtime_error("Unknown option: " + parser.unknown_flags().front());
    }
    opts.include_private = parser.has_flag("--include-private") || cfg_flag("--include-private");
    opts.show_skipped = parser.has_flag("--show-skipped") || cfg_flag("--show-skipped");
    opts.show_version = parser.has_flag("--show-version") || cfg_flag("--show-version");
    opts.remove_lock = parser.has_flag("--remove-lock") || cfg_flag("--remove-lock");
    opts.check_only = parser.has_flag("--check-only") || cfg_flag("--check-only");
    opts.hash_check = !(parser.has_flag("--no-hash-check") || cfg_flag("--no-hash-check"));
    opts.force_pull = parser.has_flag("--force-pull") || parser.has_flag("--discard-dirty") ||
                      cfg_flag("--force-pull") || cfg_flag("--discard-dirty");
    if (parser.has_flag("--verbose") || cfg_flag("--verbose"))
        opts.log_level = LogLevel::DEBUG;
    if (parser.has_flag("--log-level") || cfg_opts.count("--log-level")) {
        std::string val = parser.get_option("--log-level");
        if (val.empty())
            val = cfg_opt("--log-level");
        if (val.empty())
            throw std::runtime_error("--log-level requires a value");
        for (auto& c : val)
            c = toupper(static_cast<unsigned char>(c));
        if (val == "DEBUG")
            opts.log_level = LogLevel::DEBUG;
        else if (val == "INFO")
            opts.log_level = LogLevel::INFO;
        else if (val == "WARNING" || val == "WARN")
            opts.log_level = LogLevel::WARNING;
        else if (val == "ERROR")
            opts.log_level = LogLevel::ERR;
        else
            throw std::runtime_error("Invalid log level: " + val);
    }
    opts.concurrency = std::thread::hardware_concurrency();
    if (opts.concurrency == 0)
        opts.concurrency = 1;
    bool ok = false;
    if (cfg_opts.count("--threads")) {
        opts.concurrency = parse_size_t(cfg_opt("--threads"), 1, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --threads");
    }
    if (cfg_flag("--single-thread"))
        opts.concurrency = 1;
    if (cfg_opts.count("--concurrency")) {
        opts.concurrency = parse_size_t(cfg_opt("--concurrency"), 1, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --concurrency");
    }
    if (parser.has_flag("--threads")) {
        opts.concurrency = parse_size_t(parser, "--threads", 1, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --threads");
    }
    if (parser.has_flag("--single-thread"))
        opts.concurrency = 1;
    if (parser.has_flag("--concurrency")) {
        opts.concurrency = parse_size_t(parser, "--concurrency", 1, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --concurrency");
    }
    if (cfg_opts.count("--max-threads")) {
        opts.max_threads = parse_size_t(cfg_opt("--max-threads"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --max-threads");
    }
    if (parser.has_flag("--max-threads")) {
        opts.max_threads = parse_size_t(parser, "--max-threads", 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --max-threads");
    }
    if (cfg_opts.count("--cpu-percent")) {
        std::string v = cfg_opt("--cpu-percent");
        if (!v.empty() && v.back() == '%')
            v.pop_back();
        opts.cpu_percent_limit = parse_int(v, 0, 100, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-percent");
    }
    if (parser.has_flag("--cpu-percent")) {
        std::string v = parser.get_option("--cpu-percent");
        if (!v.empty() && v.back() == '%')
            v.pop_back();
        opts.cpu_percent_limit = parse_int(v, 0, 100, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-percent");
    }
    if (cfg_opts.count("--cpu-cores")) {
        opts.cpu_core_mask = parse_ull(cfg_opt("--cpu-cores"), 0, ULLONG_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-cores");
    }
    if (parser.has_flag("--cpu-cores")) {
        opts.cpu_core_mask = parse_ull(parser, "--cpu-cores", 0, ULLONG_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-cores");
    }
    if (cfg_opts.count("--mem-limit")) {
        opts.mem_limit = parse_size_t(cfg_opt("--mem-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --mem-limit");
    }
    if (parser.has_flag("--mem-limit")) {
        opts.mem_limit = parse_size_t(parser, "--mem-limit", 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --mem-limit");
    }
    if (cfg_opts.count("--download-limit")) {
        opts.download_limit = parse_size_t(cfg_opt("--download-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --download-limit");
    }
    if (parser.has_flag("--download-limit")) {
        opts.download_limit = parse_size_t(parser, "--download-limit", 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --download-limit");
    }
    if (cfg_opts.count("--upload-limit")) {
        opts.upload_limit = parse_size_t(cfg_opt("--upload-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --upload-limit");
    }
    if (parser.has_flag("--upload-limit")) {
        opts.upload_limit = parse_size_t(parser, "--upload-limit", 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --upload-limit");
    }
    if (cfg_opts.count("--disk-limit")) {
        opts.disk_limit = parse_size_t(cfg_opt("--disk-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --disk-limit");
    }
    if (parser.has_flag("--disk-limit")) {
        opts.disk_limit = parse_size_t(parser, "--disk-limit", 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --disk-limit");
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
    opts.cpu_tracker = !cfg_flag("--no-cpu-tracker");
    opts.mem_tracker = !cfg_flag("--no-mem-tracker");
    opts.thread_tracker = !cfg_flag("--no-thread-tracker");
    opts.net_tracker = cfg_flag("--net-tracker");
    if (parser.has_flag("--no-cpu-tracker"))
        opts.cpu_tracker = false;
    if (parser.has_flag("--no-mem-tracker"))
        opts.mem_tracker = false;
    if (parser.has_flag("--no-thread-tracker"))
        opts.thread_tracker = false;
    if (parser.has_flag("--net-tracker"))
        opts.net_tracker = true;
    opts.debug_memory = cfg_flag("--debug-memory") || parser.has_flag("--debug-memory");
    opts.dump_state = cfg_flag("--dump-state") || parser.has_flag("--dump-state");
    if (cfg_opts.count("--dump-large")) {
        opts.dump_threshold = parse_size_t(cfg_opt("--dump-large"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --dump-large");
    }
    if (parser.has_flag("--dump-large")) {
        opts.dump_threshold = parse_size_t(parser, "--dump-large", 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --dump-large");
    }
    if (cfg_opts.count("--interval")) {
        opts.interval = parse_int(cfg_opt("--interval"), 1, INT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --interval");
    }
    if (parser.has_flag("--interval")) {
        opts.interval = parse_int(parser, "--interval", 1, INT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --interval");
    }
    if (cfg_opts.count("--refresh-rate")) {
        int v = parse_int(cfg_opt("--refresh-rate"), 1, INT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --refresh-rate");
        opts.refresh_ms = std::chrono::milliseconds(v);
    }
    if (parser.has_flag("--refresh-rate")) {
        int v = parse_int(parser, "--refresh-rate", 1, INT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --refresh-rate");
        opts.refresh_ms = std::chrono::milliseconds(v);
    }
    if (cfg_opts.count("--cpu-poll")) {
        opts.cpu_poll_sec = parse_uint(cfg_opt("--cpu-poll"), 1u, UINT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-poll");
    }
    if (parser.has_flag("--cpu-poll")) {
        opts.cpu_poll_sec = parse_uint(parser, "--cpu-poll", 1u, UINT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-poll");
    }
    if (cfg_opts.count("--mem-poll")) {
        opts.mem_poll_sec = parse_uint(cfg_opt("--mem-poll"), 1u, UINT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --mem-poll");
    }
    if (parser.has_flag("--mem-poll")) {
        opts.mem_poll_sec = parse_uint(parser, "--mem-poll", 1u, UINT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --mem-poll");
    }
    if (cfg_opts.count("--thread-poll")) {
        opts.thread_poll_sec = parse_uint(cfg_opt("--thread-poll"), 1u, UINT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --thread-poll");
    }
    if (parser.has_flag("--thread-poll")) {
        opts.thread_poll_sec = parse_uint(parser, "--thread-poll", 1u, UINT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --thread-poll");
    }
    if (parser.has_flag("--log-dir") || cfg_opts.count("--log-dir")) {
        std::string val = parser.get_option("--log-dir");
        if (val.empty())
            val = cfg_opt("--log-dir");
        if (val.empty())
            throw std::runtime_error("--log-dir requires a path");
        opts.log_dir = val;
    }
    if (parser.has_flag("--log-file") || cfg_opts.count("--log-file")) {
        std::string val = parser.get_option("--log-file");
        if (val.empty())
            val = cfg_opt("--log-file");
        opts.log_file = val;
    }
    opts.root = parser.positional().empty() ? fs::path() : fs::path(parser.positional().front());
    for (const auto& val : parser.get_all_options("--ignore"))
        opts.ignore_dirs.push_back(val);
    return opts;
}
