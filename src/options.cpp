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

void parse_service_options(Options& opts, ArgParser& parser,
                           const std::function<bool(const std::string&)>& cfg_flag,
                           const std::function<std::string(const std::string&)>& cfg_opt,
                           const std::map<std::string, std::string>& cfg_opts) {
    struct ControlFlag {
        const char* flag;
        bool ServiceOptions::* state;
        std::string ServiceOptions::* name;
    };
    struct BoolFlag {
        const char* flag;
        bool ServiceOptions::* state;
    };
    const BoolFlag bool_flags[] = {
        {"--install-daemon", &ServiceOptions::install_daemon},
        {"--uninstall-daemon", &ServiceOptions::uninstall_daemon},
        {"--install-service", &ServiceOptions::install_service},
        {"--uninstall-service", &ServiceOptions::uninstall_service},
        {"--service-status", &ServiceOptions::service_status},
        {"--daemon-status", &ServiceOptions::daemon_status},
        {"--show-service", &ServiceOptions::show_service},
        {"--kill-all", &ServiceOptions::kill_all},
        {"--kill-on-sleep", &ServiceOptions::kill_on_sleep},
        {"--list-instances", &ServiceOptions::list_instances},
    };
    for (const auto& bf : bool_flags)
        opts.service.*(bf.state) = parser.has_flag(bf.flag) || cfg_flag(bf.flag);

    if (opts.service.install_daemon) {
        std::string val = parser.get_option("--install-daemon");
        if (!val.empty())
            opts.service.daemon_name = val;
    }
    if (opts.service.uninstall_daemon) {
        std::string val = parser.get_option("--uninstall-daemon");
        if (!val.empty())
            opts.service.daemon_name = val;
    }
    if (opts.service.install_service) {
        std::string val = parser.get_option("--install-service");
        if (!val.empty())
            opts.service.service_name = val;
    }
    if (opts.service.uninstall_service) {
        std::string val = parser.get_option("--uninstall-service");
        if (!val.empty())
            opts.service.service_name = val;
    }

    opts.service.list_services = parser.has_flag("--list-services") ||
                                 parser.has_flag("--list-daemons") || cfg_flag("--list-services") ||
                                 cfg_flag("--list-daemons");

    struct ValueOpt {
        const char* flag;
        std::string ServiceOptions::* member;
    };
    const ValueOpt value_opts[] = {
        {"--daemon-config", &ServiceOptions::daemon_config},
        {"--service-config", &ServiceOptions::service_config},
        {"--service-name", &ServiceOptions::service_name},
        {"--daemon-name", &ServiceOptions::daemon_name},
    };
    for (const auto& vo : value_opts) {
        if (parser.has_flag(vo.flag) || cfg_opts.count(vo.flag)) {
            std::string val = parser.get_option(vo.flag);
            if (val.empty())
                val = cfg_opt(vo.flag);
            opts.service.*(vo.member) = val;
        }
    }

    const ControlFlag daemon_controls[] = {
        {"--start-daemon", &ServiceOptions::start_daemon, &ServiceOptions::start_daemon_name},
        {"--stop-daemon", &ServiceOptions::stop_daemon, &ServiceOptions::stop_daemon_name},
        {"--restart-daemon", &ServiceOptions::restart_daemon, &ServiceOptions::restart_daemon_name},
    };
    for (const auto& c : daemon_controls) {
        if (parser.has_flag(c.flag) || cfg_flag(c.flag)) {
            opts.service.*(c.state) = true;
            std::string val = parser.get_option(c.flag);
            if (val.empty())
                val = cfg_opt(c.flag);
            opts.service.*(c.name) = val;
        }
    }
    const ControlFlag service_controls[] = {
        {"--start-service", &ServiceOptions::start_service, &ServiceOptions::start_service_name},
        {"--stop-service", &ServiceOptions::stop_service, &ServiceOptions::stop_service_name},
        {"--restart-service", &ServiceOptions::restart_service,
         &ServiceOptions::restart_service_name},
    };
    for (const auto& c : service_controls) {
        if (parser.has_flag(c.flag) || cfg_flag(c.flag)) {
            opts.service.*(c.state) = true;
            std::string val = parser.get_option(c.flag);
            if (val.empty())
                val = cfg_opt(c.flag);
            opts.service.*(c.name) = val;
        }
    }

    struct ForceFlag {
        const char* flag;
        bool ServiceOptions::* state;
    };
    const ForceFlag force_flags[] = {
        {"--force-stop-daemon", &ServiceOptions::force_stop_daemon},
        {"--force-restart-daemon", &ServiceOptions::force_restart_daemon},
        {"--force-stop-service", &ServiceOptions::force_stop_service},
        {"--force-restart-service", &ServiceOptions::force_restart_service},
    };
    for (const auto& ff : force_flags)
        opts.service.*(ff.state) = parser.has_flag(ff.flag) || cfg_flag(ff.flag);

    if (parser.has_flag("--attach") || cfg_opts.count("--attach")) {
        std::string val = parser.get_option("--attach");
        if (val.empty())
            val = cfg_opt("--attach");
        if (val.empty())
            throw std::runtime_error("--attach requires a name");
        opts.service.attach_name = val;
    }
    opts.service.run_background = parser.has_flag("--background") || cfg_opts.count("--background");
    if (opts.service.run_background) {
        std::string val = parser.get_option("--background");
        if (val.empty())
            val = cfg_opt("--background");
        if (val.empty())
            throw std::runtime_error("--background requires a name");
        opts.service.attach_name = val;
    }
    if (parser.has_flag("--reattach") || cfg_opts.count("--reattach")) {
        std::string val = parser.get_option("--reattach");
        if (val.empty())
            val = cfg_opt("--reattach");
        if (val.empty())
            throw std::runtime_error("--reattach requires a name");
        opts.service.attach_name = val;
        opts.service.reattach = true;
    }

    if (opts.service.start_service_name.empty())
        opts.service.start_service_name = opts.service.service_name;
    if (opts.service.stop_service_name.empty())
        opts.service.stop_service_name = opts.service.service_name;
    if (opts.service.restart_service_name.empty())
        opts.service.restart_service_name = opts.service.service_name;
    if (opts.service.start_daemon_name.empty())
        opts.service.start_daemon_name = opts.service.daemon_name;
    if (opts.service.stop_daemon_name.empty())
        opts.service.stop_daemon_name = opts.service.daemon_name;
    if (opts.service.restart_daemon_name.empty())
        opts.service.restart_daemon_name = opts.service.daemon_name;
}

void parse_tracker_options(Options& opts, ArgParser& parser,
                           const std::function<bool(const std::string&)>& cfg_flag) {
    struct TrackerFlag {
        const char* flag;
        bool negative;
        bool Options::* member;
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
        size_t ResourceLimits::* member;
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

Options parse_options(int argc, char* argv[]) {
    fs::path config_file;
    // Parse config file option first
    const std::set<std::string> pre_known{"--config-yaml", "--config-json"};
    const std::map<char, std::string> pre_short{{'y', "--config-yaml"}, {'j', "--config-json"}};
    ArgParser pre_parser(argc, argv, pre_known, pre_short);
    std::map<std::string, std::string> cfg_opts;
    std::map<std::string, std::map<std::string, std::string>> cfg_repo_opts;
    if (pre_parser.has_flag("--config-yaml")) {
        std::string cfg = pre_parser.get_option("--config-yaml");
        if (cfg.empty())
            throw std::runtime_error("--config-yaml requires a file");
        std::string err;
        if (!load_yaml_config(cfg, cfg_opts, cfg_repo_opts, err))
            throw std::runtime_error("Failed to load config: " + err);
        config_file = cfg;
    }
    if (pre_parser.has_flag("--config-json")) {
        std::string cfg = pre_parser.get_option("--config-json");
        if (cfg.empty())
            throw std::runtime_error("--config-json requires a file");
        std::string err;
        if (!load_json_config(cfg, cfg_opts, cfg_repo_opts, err))
            throw std::runtime_error("Failed to load config: " + err);
        config_file = cfg;
    }

    // Look for automatic config files if requested
    const std::set<std::string> auto_known{"--root", "--auto-config"};
    const std::map<char, std::string> auto_short{{'o', "--root"}};
    ArgParser auto_parser(argc, argv, auto_known, auto_short);
    auto cfg_flag_pre = [&](const std::string& k) {
        auto it = cfg_opts.find(k);
        if (it == cfg_opts.end())
            return false;
        std::string v = it->second;
        std::transform(v.begin(), v.end(), v.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return v == "" || v == "1" || v == "true" || v == "yes";
    };

    bool want_auto = auto_parser.has_flag("--auto-config") || cfg_flag_pre("--auto-config");
    fs::path root_hint;
    if (auto_parser.has_flag("--root"))
        root_hint = auto_parser.get_option("--root");
    else if (!auto_parser.positional().empty())
        root_hint = auto_parser.positional().front();
    if (root_hint.empty() && cfg_opts.count("--root"))
        root_hint = cfg_opts["--root"];

    if (want_auto) {
        auto find_cfg = [](const fs::path& dir) -> fs::path {
            if (dir.empty())
                return {};
            fs::path y = dir / ".autogitpull.yaml";
            if (fs::exists(y))
                return y;
            fs::path j = dir / ".autogitpull.json";
            if (fs::exists(j))
                return j;
            return {};
        };
        fs::path exe_dir;
        if (argv && argv[0])
            exe_dir = fs::absolute(argv[0]).parent_path();
        fs::path cfg_path = find_cfg(root_hint);
        if (cfg_path.empty())
            cfg_path = find_cfg(fs::current_path());
        if (cfg_path.empty())
            cfg_path = find_cfg(exe_dir);
        if (!cfg_path.empty()) {
            std::string err;
            if (cfg_path.extension() == ".yaml") {
                if (!load_yaml_config(cfg_path.string(), cfg_opts, cfg_repo_opts, err))
                    throw std::runtime_error("Failed to load config: " + err);
            } else {
                if (!load_json_config(cfg_path.string(), cfg_opts, cfg_repo_opts, err))
                    throw std::runtime_error("Failed to load config: " + err);
            }
            config_file = cfg_path;
        }
    }

    const std::set<std::string> known{"--include-private",
                                      "--show-skipped",
                                      "--show-notgit",
                                      "--show-version",
                                      "--version",
                                      "--root",
                                      "--remote",
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
                                      "--max-log-size",
                                      "--concurrency",
                                      "--check-only",
                                      "--no-hash-check",
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
                                      "--webhook-url",
                                      "--webhook-secret",
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
        bool ok = false;
        auto dur = parse_duration(val, ok);
        if (!ok || dur.count() < 1 || dur.count() > INT_MAX)
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
        bool ok = false;
        if (comma == std::string::npos) {
            opts.service.respawn_max = parse_int(val, 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid value for --respawn-limit");
        } else {
            opts.service.respawn_max = parse_int(val.substr(0, comma), 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid value for --respawn-limit");
            bool ok2 = false;
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
        bool ok = false;
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
            bool ok = false;
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
        bool ok = false;
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
    if (parser.has_flag("--webhook-url") || cfg_opts.count("--webhook-url")) {
        std::string val = parser.get_option("--webhook-url");
        if (val.empty())
            val = cfg_opt("--webhook-url");
        opts.webhook_url = val;
    }
    if (parser.has_flag("--webhook-secret") || cfg_opts.count("--webhook-secret")) {
        std::string val = parser.get_option("--webhook-secret");
        if (val.empty())
            val = cfg_opt("--webhook-secret");
        if (!val.empty())
            opts.webhook_secret = val;
    }
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
            bool ok = false;
            opts.wait_empty_limit = parse_int(val, 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid value for --wait-empty");
        }
    }
    opts.silent = parser.has_flag("--silent") || cfg_flag("--silent");
    opts.recursive_scan = parser.has_flag("--recursive") || cfg_flag("--recursive");
    opts.show_help = parser.has_flag("--help");
    opts.print_version = parser.has_flag("--version");
    opts.hard_reset = parser.has_flag("--hard-reset") || cfg_flag("--hard-reset");
    opts.confirm_reset = parser.has_flag("--confirm-reset") || cfg_flag("--confirm-reset");
    opts.confirm_alert = parser.has_flag("--confirm-alert") || cfg_flag("--confirm-alert");
    opts.sudo_su = parser.has_flag("--sudo-su") || cfg_flag("--sudo-su");
    opts.mutant_mode = parser.has_flag("--mutant") || parser.has_flag("--recover-mutant") ||
                       cfg_flag("--mutant") || cfg_flag("--recover-mutant");
    opts.confirm_mutant = parser.has_flag("--confirm-mutant") || cfg_flag("--confirm-mutant");
    opts.recover_mutant = parser.has_flag("--recover-mutant") || cfg_flag("--recover-mutant");
    if (parser.has_flag("--mutant-config") || cfg_opts.count("--mutant-config")) {
        std::string val = parser.get_option("--mutant-config");
        if (val.empty())
            val = cfg_opt("--mutant-config");
        if (!val.empty())
            opts.mutant_config = val;
    }
    if (opts.mutant_mode) {
        if (!(opts.confirm_mutant || opts.sudo_su))
            throw std::runtime_error("--mutant requires --confirm-mutant or --sudo-su");
        opts.service.persist = true;
        if (opts.recover_mutant) {
            opts.service.reattach = true;
            if (opts.service.attach_name.empty())
                opts.service.attach_name = "mutant";
        } else {
            opts.service.run_background = true;
            if (opts.service.attach_name.empty())
                opts.service.attach_name = "mutant";
        }
    }
    opts.add_ignore = parser.has_flag("--add-ignore") || cfg_flag("--add-ignore");
    if (opts.add_ignore)
        opts.add_ignore_repo = parser.get_option("--add-ignore");
    opts.remove_ignore = parser.has_flag("--remove-ignore") || cfg_flag("--remove-ignore");
    if (opts.remove_ignore)
        opts.remove_ignore_repo = parser.get_option("--remove-ignore");
    opts.clear_ignores = parser.has_flag("--clear-ignores") || cfg_flag("--clear-ignores");
    opts.find_ignores = parser.has_flag("--find-ignores") || cfg_flag("--find-ignores");
    if (parser.has_flag("--depth") || cfg_opts.count("--depth")) {
        std::string val = parser.get_option("--depth");
        if (val.empty())
            val = cfg_opt("--depth");
        bool ok = false;
        unsigned int d = parse_uint(val, 0, UINT_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --depth");
        opts.depth = d;
    }
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
            c = toupper(static_cast<unsigned char>(c));
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
    bool ok = false;
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
    if (parser.has_flag("--log-dir") || cfg_opts.count("--log-dir")) {
        std::string val = parser.get_option("--log-dir");
        if (val.empty())
            val = cfg_opt("--log-dir");
        if (val.empty())
            throw std::runtime_error("--log-dir requires a path");
        opts.logging.log_dir = val;
    }
    if (parser.has_flag("--log-file") || cfg_opts.count("--log-file")) {
        std::string val = parser.get_option("--log-file");
        if (val.empty())
            val = cfg_opt("--log-file");
        opts.logging.log_file = val;
    }
    if (parser.has_flag("--ssh-public-key") || cfg_opts.count("--ssh-public-key")) {
        std::string val = parser.get_option("--ssh-public-key");
        if (val.empty())
            val = cfg_opt("--ssh-public-key");
        if (val.empty())
            throw std::runtime_error("--ssh-public-key requires a path");
        opts.ssh_public_key = val;
    }
    if (parser.has_flag("--ssh-private-key") || cfg_opts.count("--ssh-private-key")) {
        std::string val = parser.get_option("--ssh-private-key");
        if (val.empty())
            val = cfg_opt("--ssh-private-key");
        if (val.empty())
            throw std::runtime_error("--ssh-private-key requires a path");
        opts.ssh_private_key = val;
    }
    if (parser.has_flag("--credential-file") || cfg_opts.count("--credential-file")) {
        std::string val = parser.get_option("--credential-file");
        if (val.empty())
            val = cfg_opt("--credential-file");
        if (val.empty())
            throw std::runtime_error("--credential-file requires a path");
        opts.credential_file = val;
    }
    if (parser.has_flag("--max-log-size") || cfg_opts.count("--max-log-size")) {
        std::string val = parser.get_option("--max-log-size");
        if (val.empty())
            val = cfg_opt("--max-log-size");
        bool ok = false;
        opts.logging.max_log_size = parse_bytes(val, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --max-log-size");
    }
    opts.show_commit_date = parser.has_flag("--show-commit-date") || cfg_flag("--show-commit-date");
    opts.show_commit_author =
        parser.has_flag("--show-commit-author") || cfg_flag("--show-commit-author");
    opts.show_datetime_line =
        !(parser.has_flag("--hide-date-time") || cfg_flag("--hide-date-time"));
    opts.show_header = !(parser.has_flag("--hide-header") || cfg_flag("--hide-header"));
    opts.show_vmem = parser.has_flag("--vmem") || cfg_flag("--vmem");
    opts.no_colors = parser.has_flag("--no-colors") || cfg_flag("--no-colors");
    if (parser.has_flag("--color") || cfg_opts.count("--color")) {
        std::string val = parser.get_option("--color");
        if (val.empty())
            val = cfg_opt("--color");
        opts.custom_color = val;
    }
    if (parser.has_flag("--theme") || cfg_opts.count("--theme")) {
        std::string val = parser.get_option("--theme");
        if (val.empty())
            val = cfg_opt("--theme");
        opts.theme_file = val;
        if (!val.empty()) {
            std::string err;
            if (!load_theme(val, opts.theme, err))
                throw std::runtime_error("Failed to load theme: " + err);
        }
    }
    if (parser.has_flag("--row-order") || cfg_opts.count("--row-order")) {
        std::string val = parser.get_option("--row-order");
        if (val.empty())
            val = cfg_opt("--row-order");
        if (val == "alpha")
            opts.sort_mode = Options::ALPHA;
        else if (val == "reverse")
            opts.sort_mode = Options::REVERSE;
        else if (val == "updated")
            opts.sort_mode = Options::UPDATED;
        else
            throw std::runtime_error("Invalid value for --row-order");
    }
    opts.logging.json_log = parser.has_flag("--json-log") || cfg_flag("--json-log");
    opts.logging.use_syslog = parser.has_flag("--syslog") || cfg_flag("--syslog");
    if (parser.has_flag("--syslog-facility") || cfg_opts.count("--syslog-facility")) {
        std::string val = parser.get_option("--syslog-facility");
        if (val.empty())
            val = cfg_opt("--syslog-facility");
        bool ok2 = false;
        int fac = parse_int(val, 0, INT_MAX, ok2);
        if (!ok2)
            throw std::runtime_error("Invalid value for --syslog-facility");
        opts.logging.syslog_facility = fac;
    }
    if (parser.has_flag("--pull-timeout") || cfg_opts.count("--pull-timeout")) {
        std::string val = parser.get_option("--pull-timeout");
        if (val.empty())
            val = cfg_opt("--pull-timeout");
        bool ok2 = false;
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
    if (parser.has_flag("--remote") || cfg_opts.count("--remote")) {
        std::string val = parser.get_option("--remote");
        if (val.empty())
            val = cfg_opt("--remote");
        if (val.empty())
            throw std::runtime_error("--remote requires a name");
        opts.remote_name = val;
    }
    if (parser.has_flag("--root") || cfg_opts.count("--root")) {
        std::string val = parser.get_option("--root");
        if (val.empty())
            val = cfg_opt("--root");
        if (val.empty())
            throw std::runtime_error("--root requires a path");
        opts.root = val;
    } else {
        opts.root =
            parser.positional().empty() ? fs::path() : fs::path(parser.positional().front());
    }
    if (opts.root.empty() && !opts.show_help && !opts.print_version && !opts.service.show_service &&
        ((opts.service.attach_name.empty() && !opts.service.reattach) ||
         opts.service.run_background || persist_flag))
        throw std::runtime_error("Root path required");
    if (persist_flag) {
        opts.service.persist = true;
        std::string name = persist_val;
        if (name.empty())
            name = opts.root.filename().string();
        if (opts.service.attach_name.empty())
            opts.service.attach_name = name;
    }
    for (const auto& val : parser.get_all_options("--include-dir"))
        opts.include_dirs.push_back(val);
    for (const auto& val : parser.get_all_options("--ignore"))
        opts.ignore_dirs.push_back(val);

    for (const auto& [repo, values] : cfg_repo_opts) {
        RepoOptions ro;
        auto rflag = [&](const std::string& k) {
            auto it = values.find(k);
            if (it == values.end())
                return false;
            std::string v = it->second;
            std::transform(v.begin(), v.end(), v.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            return v == "" || v == "1" || v == "true" || v == "yes";
        };
        auto ropt = [&](const std::string& k) {
            auto it = values.find(k);
            if (it != values.end())
                return it->second;
            return std::string();
        };
        bool ok = false;
        if (rflag("--force-pull") || rflag("--discard-dirty"))
            ro.force_pull = true;
        if (rflag("--exclude"))
            ro.exclude = true;
        if (rflag("--check-only"))
            ro.check_only = true;
        if (values.count("--download-limit")) {
            size_t bytes = parse_bytes(ropt("--download-limit"), 0, SIZE_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo download-limit");
            ro.download_limit = bytes / 1024ull;
        }
        if (values.count("--upload-limit")) {
            size_t bytes = parse_bytes(ropt("--upload-limit"), 0, SIZE_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo upload-limit");
            ro.upload_limit = bytes / 1024ull;
        }
        if (values.count("--disk-limit")) {
            size_t bytes = parse_bytes(ropt("--disk-limit"), 0, SIZE_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo disk-limit");
            ro.disk_limit = bytes / 1024ull;
        }
        if (values.count("--cpu-limit")) {
            double pct = parse_double(ropt("--cpu-limit"), 0.0, 100.0, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo cpu-limit");
            ro.cpu_limit = pct;
        }
        if (values.count("--max-runtime")) {
            auto dur = parse_duration(ropt("--max-runtime"), ok);
            if (!ok || dur.count() < 1 || dur.count() > INT_MAX)
                throw std::runtime_error("Invalid per-repo max-runtime");
            ro.max_runtime = dur;
        }
        if (values.count("--pull-timeout")) {
            auto dur = parse_duration(ropt("--pull-timeout"), ok);
            if (!ok || dur.count() < 1 || dur.count() > INT_MAX)
                throw std::runtime_error("Invalid per-repo pull-timeout");
            ro.pull_timeout = dur;
        }
        opts.repo_settings[fs::path(repo)] = ro;
    }

    if ((opts.rerun_last || opts.save_args || opts.enable_history) &&
        opts.history_file == ".autogitpull.config") {
        auto find_hist = [](const fs::path& dir) -> fs::path {
            if (dir.empty())
                return {};
            fs::path c = dir / ".autogitpull.config";
            if (fs::exists(c))
                return c;
            return {};
        };
        fs::path exe_dir;
        if (argv && argv[0])
            exe_dir = fs::absolute(argv[0]).parent_path();
        fs::path hist = find_hist(opts.root);
        if (hist.empty())
            hist = find_hist(fs::current_path());
        if (hist.empty())
            hist = find_hist(exe_dir);
        if (!hist.empty())
            opts.history_file = hist.string();
    }
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
