#include <algorithm>
#include <chrono>
#include <cctype>
#include <climits>
#include <filesystem>
#include <map>
#include <functional>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "arg_parser.hpp"
#include "config_utils.hpp"
#include "options.hpp"
#include "parse_utils.hpp"

namespace fs = std::filesystem;

// Note: service/tracker/limits parsing implementations are defined in
// src/options_service.cpp, src/options_tracker.cpp, and
// src/options_limits.cpp, respectively.

Options parse_options(int argc, char* argv[]) {
    fs::path config_file;
    std::map<std::string, std::string> cfg_opts;
    std::map<std::string, std::map<std::string, std::string>> cfg_repo_opts;
    load_config_and_auto(argc, argv, cfg_opts, cfg_repo_opts, config_file);

    const std::set<std::string> known{"--include-private",
                                      "--show-skipped",
                                      "--show-notgit",
                                      "--show-version",
                                      "--version",
                                      "--root",
                                      "--remote",
                                      "--pull-ref",
                                      "--interval",
                                      "--refresh-rate",
                                      "--cpu-poll",
                                      "--mem-poll",
                                      "--thread-poll",
                                      "--log-dir",
                                      "--log-file",
                                      "--ssh-public-key",
                                      "--ssh-private-key",
                                      "--credential-file",
                                      "--proxy",
                                      "--max-log-size",
                                      "--concurrency",
                                      "--check-only",
                                      "--no-hash-check",
                                      "--dry-run",
                                      "--log-level",
                                      "--verbose",
                                      "--max-threads",
                                      "--cpu-percent",
                                      "--cpu-cores",
                                      "--mem-limit",
                                      "--no-cpu-tracker",
                                      "--no-mem-tracker",
                                      "--no-thread-tracker",
                                      "--help",
                                      "--threads",
                                      "--single-thread",
                                      "--net-tracker",
                                      "--download-limit",
                                      "--upload-limit",
                                      "--disk-limit",
                                      "--cpu-limit",
                                      "--total-traffic-limit",
                                      "--max-depth",
                                      "--cli",
                                      "--single-run",
                                      "--single-repo",
                                      "--silent",
                                      "--recursive",
                                      "--config-yaml",
                                      "--config-json",
                                      "--ignore",
                                      "--include-dir",
                                      "--force-pull",
                                      "--exclude",
                                      "--discard-dirty",
                                      "--post-pull-hook",
                                      "--debug-memory",
                                      "--dump-state",
                                      "--dump-large",
                                      "--install-daemon",
                                      "--uninstall-daemon",
                                      "--daemon-config",
                                      "--install-service",
                                      "--uninstall-service",
                                      "--start-service",
                                      "--stop-service",
                                      "--force-stop-service",
                                      "--restart-service",
                                      "--force-restart-service",
                                      "--service-config",
                                      "--service-name",
                                      "--daemon-name",
                                      "--start-daemon",
                                      "--stop-daemon",
                                      "--force-stop-daemon",
                                      "--restart-daemon",
                                      "--force-restart-daemon",
                                      "--service-status",
                                      "--daemon-status",
                                      "--show-service",
                                      "--attach",
                                      "--background",
                                      "--reattach",
                                      "--remove-lock",
                                      "--ignore-lock",
                                      "--show-runtime",
                                      "--show-repo-count",
                                      "--max-runtime",
                                      "--persist",
                                      "--respawn-limit",
                                      "--respawn-delay",
                                      "--kill-all",
                                      "--kill-on-sleep",
                                      "--list-instances",
                                      "--list-services",
                                      "--list-daemons",
                                      "--rescan-new",
                                      "--show-commit-date",
                                      "--show-commit-author",
                                      "--hide-date-time",
                                      "--hide-header",
                                      "--vmem",
                                      "--no-colors",
                                      "--color",
                                      "--row-order",
                                      "--syslog",
                                      "--syslog-facility",
                                      "--json-log",
                                      "--compress-logs",
                                      "--pull-timeout",
                                      "--exit-on-timeout",
                                      "--dont-skip-timeouts",
                                      "--dont-skip-unavailable",
                                      "--retry-skipped",
                                      "--reset-skipped",
                                      "--skip-accessible-errors",
                                      "--keep-first-valid",
                                      "--wait-empty",
                                      "--updated-since",
                                      "--auto-config",
                                      "--auto-reload-config",
                                      "--rerun-last",
                                      "--save-args",
                                      "--enable-history",
                                      "--enable-hotkeys",
                                      "--session-dates-only",
                                      "--print-skipped",
                                      "--show-pull-author",
                                      "--censor-names",
                                      "--censor-char",
                                      "--keep-first",
                                      "--hard-reset",
                                      "--confirm-reset",
                                      "--confirm-alert",
                                      "--sudo-su",
                                      "--mutant",
                                      "--recover-mutant",
                                      "--confirm-mutant",
                                      "--mutant-config",
                                      "--add-ignore",
                                      "--remove-ignore",
                                      "--clear-ignores",
                                      "--find-ignores",
                                      "--depth"};
    const std::map<char, std::string> short_opts{{'p', "--include-private"},
                                                 {'k', "--show-skipped"},
                                                 {'v', "--show-version"},
                                                 {'V', "--version"},
                                                 {'i', "--interval"},
                                                 {'r', "--refresh-rate"},
                                                 {'d', "--log-dir"},
                                                 {'l', "--log-file"},
                                                 {'y', "--config-yaml"},
                                                 {'j', "--config-json"},
                                                 {'c', "--cli"},
                                                 {'s', "--silent"},
                                                 {'D', "--max-depth"},
                                                 {'h', "--help"},
                                                 {'A', "--attach"},
                                                 {'b', "--background"},
                                                 {'B', "--reattach"},
                                                 {'R', "--remove-lock"},
                                                 {'f', "--force-pull"},
                                                 {'L', "--log-level"},
                                                 {'n', "--concurrency"},
                                                 {'t', "--threads"},
                                                 {'I', "--ignore"},
                                                 {'g', "--verbose"},
                                                 {'N', "--no-hash-check"},
                                                 {'P', "--persist"},
                                                 {'o', "--root"},
                                                 {'u', "--single-run"},
                                                 {'S', "--single-repo"},
                                                 {'C', "--no-colors"},
                                                 {'M', "--max-threads"},
                                                 {'e', "--recursive"},
                                                 {'H', "--hide-header"},
                                                 {'T', "--show-commit-date"},
                                                 {'U', "--show-commit-author"},
                                                 {'x', "--check-only"},
                                                 {'m', "--debug-memory"},
                                                 {'w', "--rescan-new"},
                                                 {'X', "--no-cpu-tracker"},
                                                 {'O', "--pull-timeout"},
                                                 {'q', "--single-thread"},
                                                 {'E', "--cpu-percent"},
                                                 {'Y', "--mem-limit"},
                                                 {'W', "--wait-empty"},
                                                 {'Z', "--show-repo-count"}};
    ArgParser parser(argc, argv, known, short_opts);
    for (const auto& kv : cfg_opts) {
        if (!known.count(kv.first))
            throw std::runtime_error("Unknown option in config: " + kv.first);
    }
    for (const auto& repo : cfg_repo_opts) {
        for (const auto& kv : repo.second) {
            if (!known.count(kv.first))
                throw std::runtime_error("Unknown option in config: " + kv.first);
        }
    }

    auto cfg_flag = [&](const std::string& k) {
        auto it = cfg_opts.find(k);
        if (it == cfg_opts.end())
            return false;
        std::string v = it->second;
        std::transform(v.begin(), v.end(), v.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return v == "" || v == "1" || v == "true" || v == "yes";
    };
    auto cfg_opt = [&](const std::string& k) {
        auto it = cfg_opts.find(k);
        if (it != cfg_opts.end())
            return it->second;
        return std::string();
    };

    Options opts;
    bool ok = false;  // reusable parse success flag for this function scope
    bool ok2 = false; // secondary parse success flag for sub-parses
    opts.cli = parser.has_flag("--cli") || cfg_flag("--cli");
    opts.single_run = parser.has_flag("--single-run") || cfg_flag("--single-run");
    opts.single_repo = parser.has_flag("--single-repo") || cfg_flag("--single-repo");
    if (opts.single_run)
        opts.cli = true;
    parse_service_options(opts, parser, cfg_flag, cfg_opt, cfg_opts);
    opts.show_runtime = parser.has_flag("--show-runtime") || cfg_flag("--show-runtime");
    opts.show_repo_count = parser.has_flag("--show-repo-count") || cfg_flag("--show-repo-count");
    if (parser.has_flag("--max-runtime") || cfg_opts.count("--max-runtime")) {
        std::string val = parser.get_option("--max-runtime");
        if (val.empty())
            val = cfg_opt("--max-runtime");
        ok2 = false;
        auto dur = parse_duration(val, ok2);
        if (!ok2 || dur.count() < 1 || dur.count() > INT_MAX)
            throw std::runtime_error("Invalid value for --max-runtime");
        opts.runtime_limit = dur;
    }
    bool persist_flag = parser.has_flag("--persist") || cfg_flag("--persist");
    std::string persist_val;
    if (persist_flag) {
        persist_val = parser.get_option("--persist");
        if (persist_val.empty())
            persist_val = cfg_opt("--persist");
    }
    if (parser.has_flag("--respawn-limit") || cfg_opts.count("--respawn-limit")) {
        std::string val = parser.get_option("--respawn-limit");
        if (val.empty())
            val = cfg_opt("--respawn-limit");
        size_t comma = val.find(',');
        ok2 = false;
        if (comma == std::string::npos) {
            opts.service.respawn_max = parse_int(val, 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid value for --respawn-limit");
        } else {
            opts.service.respawn_max = parse_int(val.substr(0, comma), 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid value for --respawn-limit");
            ok2 = false;
            int mins = parse_int(val.substr(comma + 1), 1, INT_MAX, ok2);
            if (!ok2)
                throw std::runtime_error("Invalid value for --respawn-limit");
            opts.service.respawn_window = std::chrono::minutes(mins);
        }
    }
    if (parser.has_flag("--respawn-delay") || cfg_opts.count("--respawn-delay")) {
        std::string val = parser.get_option("--respawn-delay");
        if (val.empty())
            val = cfg_opt("--respawn-delay");
        // ok2 not used here; reuse 'ok'
        opts.service.respawn_delay = parse_time_ms(val, ok);
        if (!ok || opts.service.respawn_delay.count() < 0)
            throw std::runtime_error("Invalid value for --respawn-delay");
    }
    opts.rescan_new = parser.has_flag("--rescan-new") || cfg_flag("--rescan-new");
    if (opts.rescan_new) {
        std::string val = parser.get_option("--rescan-new");
        if (val.empty() && cfg_opts.count("--rescan-new"))
            val = cfg_opt("--rescan-new");
        if (!val.empty()) {
            // reuse 'ok'
            int mins = parse_int(val, 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid value for --rescan-new");
            opts.rescan_interval = std::chrono::minutes(mins);
        }
    }
    if (parser.has_flag("--updated-since") || cfg_opts.count("--updated-since")) {
        std::string val = parser.get_option("--updated-since");
        if (val.empty())
            val = cfg_opt("--updated-since");
        // reuse 'ok'
        opts.updated_since = parse_duration(val, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --updated-since");
    }
    opts.keep_first_valid = parser.has_flag("--keep-first-valid") ||
                            cfg_flag("--keep-first-valid") || parser.has_flag("--keep-first") ||
                            cfg_flag("--keep-first");
    opts.auto_config = parser.has_flag("--auto-config") || cfg_flag("--auto-config");
    opts.auto_reload_config = parser.has_flag("--auto-reload-config");
    opts.rerun_last = parser.has_flag("--rerun-last") || cfg_flag("--rerun-last");
    opts.save_args = parser.has_flag("--save-args") || cfg_flag("--save-args");
    opts.enable_history = parser.has_flag("--enable-history") || cfg_flag("--enable-history");
    if (opts.enable_history) {
        std::string val = parser.get_option("--enable-history");
        if (val.empty())
            val = cfg_opt("--enable-history");
        if (val.empty())
            val = ".autogitpull.config";
        opts.history_file = val;
    }
    opts.enable_hotkeys = parser.has_flag("--enable-hotkeys") || cfg_flag("--enable-hotkeys");
    opts.session_dates_only =
        parser.has_flag("--session-dates-only") || cfg_flag("--session-dates-only");
    opts.cli_print_skipped = parser.has_flag("--print-skipped") || cfg_flag("--print-skipped");
    opts.show_pull_author = parser.has_flag("--show-pull-author") || cfg_flag("--show-pull-author");
    opts.censor_names = parser.has_flag("--censor-names") || cfg_flag("--censor-names");
    if (parser.has_flag("--censor-char") || cfg_opts.count("--censor-char")) {
        std::string val = parser.get_option("--censor-char");
        if (val.empty())
            val = cfg_opt("--censor-char");
        if (val.empty())
            throw std::runtime_error("--censor-char requires a character");
        opts.censor_char = val[0];
    }
    opts.wait_empty = parser.has_flag("--wait-empty") || cfg_flag("--wait-empty");
    if (opts.wait_empty) {
        std::string val = parser.get_option("--wait-empty");
        if (val.empty() && cfg_opts.count("--wait-empty"))
            val = cfg_opt("--wait-empty");
        if (!val.empty()) {
            ok2 = false;
            opts.wait_empty_limit = parse_int(val, 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid value for --wait-empty");
        }
    }
    parse_behavior_flags(opts, parser, cfg_flag, cfg_opt, cfg_opts);
    if (!parser.unknown_flags().empty()) {
        throw std::runtime_error("Unknown option: " + parser.unknown_flags().front());
    }
    opts.include_private = parser.has_flag("--include-private") || cfg_flag("--include-private");
    opts.show_skipped = parser.has_flag("--show-skipped") || cfg_flag("--show-skipped");
    opts.show_notgit = parser.has_flag("--show-notgit") || cfg_flag("--show-notgit");
    opts.show_version = parser.has_flag("--show-version") || cfg_flag("--show-version");
    opts.remove_lock = parser.has_flag("--remove-lock") || cfg_flag("--remove-lock");
    opts.ignore_lock = parser.has_flag("--ignore-lock") || cfg_flag("--ignore-lock");
    opts.check_only = parser.has_flag("--check-only") || cfg_flag("--check-only");
    opts.hash_check = !(parser.has_flag("--no-hash-check") || cfg_flag("--no-hash-check"));
    opts.dry_run = parser.has_flag("--dry-run") || cfg_flag("--dry-run");
    opts.force_pull = parser.has_flag("--force-pull") || parser.has_flag("--discard-dirty") ||
                      cfg_flag("--force-pull") || cfg_flag("--discard-dirty");
    if (parser.has_flag("--verbose") || cfg_flag("--verbose"))
        opts.logging.log_level = LogLevel::DEBUG;
    if (parser.has_flag("--log-level") || cfg_opts.count("--log-level")) {
        std::string val = parser.get_option("--log-level");
        if (val.empty())
            val = cfg_opt("--log-level");
        if (val.empty())
            throw std::runtime_error("--log-level requires a value");
        for (auto& c : val)
            c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
        if (val == "DEBUG")
            opts.logging.log_level = LogLevel::DEBUG;
        else if (val == "INFO")
            opts.logging.log_level = LogLevel::INFO;
        else if (val == "WARNING" || val == "WARN")
            opts.logging.log_level = LogLevel::WARNING;
        else if (val == "ERROR")
            opts.logging.log_level = LogLevel::ERR;
        else
            throw std::runtime_error("Invalid log level: " + val);
    }
    opts.limits.concurrency = std::thread::hardware_concurrency();
    if (opts.limits.concurrency == 0)
        opts.limits.concurrency = 1;
    // ok2 already declared near function top; ensure not redeclared here
    if (cfg_opts.count("--threads")) {
        opts.limits.concurrency = parse_size_t(cfg_opt("--threads"), 1, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --threads");
    }
    if (cfg_flag("--single-thread"))
        opts.limits.concurrency = 1;
    if (cfg_opts.count("--concurrency")) {
        opts.limits.concurrency = parse_size_t(cfg_opt("--concurrency"), 1, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --concurrency");
    }
    if (parser.has_flag("--threads")) {
        opts.limits.concurrency = parse_size_t(parser, "--threads", 1, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --threads");
    }
    if (parser.has_flag("--single-thread"))
        opts.limits.concurrency = 1;
    if (parser.has_flag("--concurrency")) {
        opts.limits.concurrency = parse_size_t(parser, "--concurrency", 1, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --concurrency");
    }
    if (cfg_opts.count("--max-threads")) {
        opts.limits.max_threads = parse_size_t(cfg_opt("--max-threads"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --max-threads");
    }
    if (parser.has_flag("--max-threads")) {
        opts.limits.max_threads = parse_size_t(parser, "--max-threads", 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --max-threads");
    }
    parse_limits(opts, parser, cfg_opt, cfg_opts);
    parse_tracker_options(opts, parser, cfg_flag);
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
    parse_timing_options(opts, parser, cfg_opt, cfg_opts);
    parse_logging_and_ui(opts, parser, cfg_flag, cfg_opt, cfg_opts);
    if (parser.has_flag("--pull-timeout") || cfg_opts.count("--pull-timeout")) {
        std::string val = parser.get_option("--pull-timeout");
        if (val.empty())
            val = cfg_opt("--pull-timeout");
        ok2 = false;
        auto dur = parse_duration(val, ok2);
        if (!ok2 || dur.count() < 1 || dur.count() > INT_MAX)
            throw std::runtime_error("Invalid value for --pull-timeout");
        opts.limits.pull_timeout = dur;
    }
    opts.limits.skip_timeout =
        !(parser.has_flag("--dont-skip-timeouts") || cfg_flag("--dont-skip-timeouts"));
    opts.skip_accessible_errors =
        parser.has_flag("--skip-accessible-errors") || cfg_flag("--skip-accessible-errors");
    opts.skip_unavailable =
        !(parser.has_flag("--dont-skip-unavailable") || cfg_flag("--dont-skip-unavailable"));
    opts.retry_skipped = parser.has_flag("--retry-skipped") || cfg_flag("--retry-skipped");
    opts.reset_skipped = parser.has_flag("--reset-skipped") || cfg_flag("--reset-skipped");
    opts.limits.exit_on_timeout =
        parser.has_flag("--exit-on-timeout") || cfg_flag("--exit-on-timeout");
    parse_root_and_repo_filters(opts, parser, cfg_opt, cfg_opts);
    finalize_persist_and_history(opts, parser, cfg_flag, cfg_opt, cfg_opts, argc, argv);
    

    parse_repo_settings(opts, cfg_repo_opts);
    
    opts.config_file = config_file;
    opts.original_args.clear();
    for (int i = 0; i < argc; ++i) {
        if (argv[i])
            opts.original_args.emplace_back(argv[i]);
    }
    return opts;
}

bool alerts_allowed(const Options& opts) {
    if (opts.confirm_alert || opts.sudo_su)
        return true;
    if (opts.interval < 15 || opts.force_pull)
        return false;
    return true;
}
