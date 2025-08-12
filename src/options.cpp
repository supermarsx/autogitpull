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
    std::map<std::string, std::map<std::string, std::string>> cfg_repo_opts;
    if (pre_parser.has_flag("--config-yaml")) {
        std::string cfg = pre_parser.get_option("--config-yaml");
        if (cfg.empty())
            throw std::runtime_error("--config-yaml requires a file");
        std::string err;
        if (!load_yaml_config(cfg, cfg_opts, cfg_repo_opts, err))
            throw std::runtime_error("Failed to load config: " + err);
    }
    if (pre_parser.has_flag("--config-json")) {
        std::string cfg = pre_parser.get_option("--config-json");
        if (cfg.empty())
            throw std::runtime_error("--config-json requires a file");
        std::string err;
        if (!load_json_config(cfg, cfg_opts, cfg_repo_opts, err))
            throw std::runtime_error("Failed to load config: " + err);
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
        }
    }

    const std::set<std::string> known{"--include-private",
                                      "--show-skipped",
                                      "--show-version",
                                      "--version",
                                      "--root",
                                      "--interval",
                                      "--refresh-rate",
                                      "--cpu-poll",
                                      "--mem-poll",
                                      "--thread-poll",
                                      "--log-dir",
                                      "--log-file",
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
                                      "--force-pull",
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
                                      "--pull-timeout",
                                      "--exit-on-timeout",
                                      "--dont-skip-timeouts",
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
    opts.install_daemon = parser.has_flag("--install-daemon") || cfg_flag("--install-daemon");
    opts.uninstall_daemon = parser.has_flag("--uninstall-daemon") || cfg_flag("--uninstall-daemon");
    opts.start_daemon = parser.has_flag("--start-daemon") || cfg_flag("--start-daemon");
    if (opts.start_daemon) {
        opts.start_daemon_name = parser.get_option("--start-daemon");
        if (opts.start_daemon_name.empty())
            opts.start_daemon_name = cfg_opt("--start-daemon");
    }
    opts.stop_daemon = parser.has_flag("--stop-daemon") || cfg_flag("--stop-daemon");
    if (opts.stop_daemon) {
        opts.stop_daemon_name = parser.get_option("--stop-daemon");
        if (opts.stop_daemon_name.empty())
            opts.stop_daemon_name = cfg_opt("--stop-daemon");
    }
    opts.force_stop_daemon =
        parser.has_flag("--force-stop-daemon") || cfg_flag("--force-stop-daemon");
    opts.restart_daemon = parser.has_flag("--restart-daemon") || cfg_flag("--restart-daemon");
    if (opts.restart_daemon) {
        opts.restart_daemon_name = parser.get_option("--restart-daemon");
        if (opts.restart_daemon_name.empty())
            opts.restart_daemon_name = cfg_opt("--restart-daemon");
    }
    opts.force_restart_daemon =
        parser.has_flag("--force-restart-daemon") || cfg_flag("--force-restart-daemon");
    opts.service_status = parser.has_flag("--service-status") || cfg_flag("--service-status");
    opts.daemon_status = parser.has_flag("--daemon-status") || cfg_flag("--daemon-status");
    if (parser.has_flag("--daemon-config") || cfg_opts.count("--daemon-config")) {
        std::string val = parser.get_option("--daemon-config");
        if (val.empty())
            val = cfg_opt("--daemon-config");
        opts.daemon_config = val;
    }
    opts.install_service = parser.has_flag("--install-service") || cfg_flag("--install-service");
    opts.uninstall_service =
        parser.has_flag("--uninstall-service") || cfg_flag("--uninstall-service");
    opts.start_service = parser.has_flag("--start-service") || cfg_flag("--start-service");
    if (opts.start_service) {
        opts.start_service_name = parser.get_option("--start-service");
        if (opts.start_service_name.empty())
            opts.start_service_name = cfg_opt("--start-service");
    }
    opts.stop_service = parser.has_flag("--stop-service") || cfg_flag("--stop-service");
    if (opts.stop_service) {
        opts.stop_service_name = parser.get_option("--stop-service");
        if (opts.stop_service_name.empty())
            opts.stop_service_name = cfg_opt("--stop-service");
    }
    opts.force_stop_service =
        parser.has_flag("--force-stop-service") || cfg_flag("--force-stop-service");
    opts.restart_service = parser.has_flag("--restart-service") || cfg_flag("--restart-service");
    if (opts.restart_service) {
        opts.restart_service_name = parser.get_option("--restart-service");
        if (opts.restart_service_name.empty())
            opts.restart_service_name = cfg_opt("--restart-service");
    }
    opts.force_restart_service =
        parser.has_flag("--force-restart-service") || cfg_flag("--force-restart-service");
    if (parser.has_flag("--service-config") || cfg_opts.count("--service-config")) {
        std::string val = parser.get_option("--service-config");
        if (val.empty())
            val = cfg_opt("--service-config");
        opts.service_config = val;
    }
    if (parser.has_flag("--service-name") || cfg_opts.count("--service-name")) {
        std::string val = parser.get_option("--service-name");
        if (val.empty())
            val = cfg_opt("--service-name");
        opts.service_name = val;
    }
    if (parser.has_flag("--daemon-name") || cfg_opts.count("--daemon-name")) {
        std::string val = parser.get_option("--daemon-name");
        if (val.empty())
            val = cfg_opt("--daemon-name");
        opts.daemon_name = val;
    }
    if (opts.start_service_name.empty())
        opts.start_service_name = opts.service_name;
    if (opts.stop_service_name.empty())
        opts.stop_service_name = opts.service_name;
    if (opts.restart_service_name.empty())
        opts.restart_service_name = opts.service_name;
    if (opts.start_daemon_name.empty())
        opts.start_daemon_name = opts.daemon_name;
    if (opts.stop_daemon_name.empty())
        opts.stop_daemon_name = opts.daemon_name;
    if (opts.restart_daemon_name.empty())
        opts.restart_daemon_name = opts.daemon_name;
    opts.show_service = parser.has_flag("--show-service") || cfg_flag("--show-service");
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
    opts.show_repo_count = parser.has_flag("--show-repo-count") || cfg_flag("--show-repo-count");
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
            opts.respawn_max = parse_int(val, 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid value for --respawn-limit");
        } else {
            opts.respawn_max = parse_int(val.substr(0, comma), 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid value for --respawn-limit");
            bool ok2 = false;
            int mins = parse_int(val.substr(comma + 1), 1, INT_MAX, ok2);
            if (!ok2)
                throw std::runtime_error("Invalid value for --respawn-limit");
            opts.respawn_window = std::chrono::minutes(mins);
        }
    }
    opts.kill_all = parser.has_flag("--kill-all") || cfg_flag("--kill-all");
    opts.kill_on_sleep = parser.has_flag("--kill-on-sleep") || cfg_flag("--kill-on-sleep");
    opts.list_instances = parser.has_flag("--list-instances") || cfg_flag("--list-instances");
    opts.list_services = parser.has_flag("--list-services") || parser.has_flag("--list-daemons") ||
                         cfg_flag("--list-services") || cfg_flag("--list-daemons");
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
    opts.auto_reload_config =
        parser.has_flag("--auto-reload-config") || cfg_flag("--auto-reload-config");
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
    opts.show_version = parser.has_flag("--show-version") || cfg_flag("--show-version");
    opts.remove_lock = parser.has_flag("--remove-lock") || cfg_flag("--remove-lock");
    opts.ignore_lock = parser.has_flag("--ignore-lock") || cfg_flag("--ignore-lock");
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
        opts.cpu_percent_limit = parse_double(v, 0.0, 100.0, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --cpu-percent");
    }
    if (parser.has_flag("--cpu-percent")) {
        std::string v = parser.get_option("--cpu-percent");
        if (!v.empty() && v.back() == '%')
            v.pop_back();
        opts.cpu_percent_limit = parse_double(v, 0.0, 100.0, ok);
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
        size_t bytes = parse_bytes(cfg_opt("--mem-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --mem-limit");
        opts.mem_limit = bytes / (1024ull * 1024ull);
    }
    if (parser.has_flag("--mem-limit")) {
        size_t bytes = parse_bytes(parser.get_option("--mem-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --mem-limit");
        opts.mem_limit = bytes / (1024ull * 1024ull);
    }
    if (cfg_opts.count("--download-limit")) {
        size_t bytes = parse_bytes(cfg_opt("--download-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --download-limit");
        opts.download_limit = bytes / 1024ull;
    }
    if (parser.has_flag("--download-limit")) {
        size_t bytes = parse_bytes(parser.get_option("--download-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --download-limit");
        opts.download_limit = bytes / 1024ull;
    }
    if (cfg_opts.count("--upload-limit")) {
        size_t bytes = parse_bytes(cfg_opt("--upload-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --upload-limit");
        opts.upload_limit = bytes / 1024ull;
    }
    if (parser.has_flag("--upload-limit")) {
        size_t bytes = parse_bytes(parser.get_option("--upload-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --upload-limit");
        opts.upload_limit = bytes / 1024ull;
    }
    if (cfg_opts.count("--disk-limit")) {
        size_t bytes = parse_bytes(cfg_opt("--disk-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --disk-limit");
        opts.disk_limit = bytes / 1024ull;
    }
    if (parser.has_flag("--disk-limit")) {
        size_t bytes = parse_bytes(parser.get_option("--disk-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --disk-limit");
        opts.disk_limit = bytes / 1024ull;
    }
    if (cfg_opts.count("--total-traffic-limit")) {
        opts.total_traffic_limit = parse_bytes(cfg_opt("--total-traffic-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --total-traffic-limit");
    }
    if (parser.has_flag("--total-traffic-limit")) {
        opts.total_traffic_limit =
            parse_bytes(parser.get_option("--total-traffic-limit"), 0, SIZE_MAX, ok);
        if (!ok)
            throw std::runtime_error("Invalid value for --total-traffic-limit");
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
        opts.cpu_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (parser.has_flag("--cpu-poll")) {
        auto dur = parse_duration(parser, "--cpu-poll", ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --cpu-poll");
        opts.cpu_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (cfg_opts.count("--mem-poll")) {
        auto dur = parse_duration(cfg_opt("--mem-poll"), ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --mem-poll");
        opts.mem_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (parser.has_flag("--mem-poll")) {
        auto dur = parse_duration(parser, "--mem-poll", ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --mem-poll");
        opts.mem_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (cfg_opts.count("--thread-poll")) {
        auto dur = parse_duration(cfg_opt("--thread-poll"), ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --thread-poll");
        opts.thread_poll_sec = static_cast<unsigned int>(dur.count());
    }
    if (parser.has_flag("--thread-poll")) {
        auto dur = parse_duration(parser, "--thread-poll", ok);
        if (!ok || dur.count() < 1 || dur.count() > UINT_MAX)
            throw std::runtime_error("Invalid value for --thread-poll");
        opts.thread_poll_sec = static_cast<unsigned int>(dur.count());
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
    if (parser.has_flag("--max-log-size") || cfg_opts.count("--max-log-size")) {
        std::string val = parser.get_option("--max-log-size");
        if (val.empty())
            val = cfg_opt("--max-log-size");
        bool ok = false;
        opts.max_log_size = parse_bytes(val, ok);
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
    if (parser.has_flag("--row-order") || cfg_opts.count("--row-order")) {
        std::string val = parser.get_option("--row-order");
        if (val.empty())
            val = cfg_opt("--row-order");
        if (val == "alpha")
            opts.sort_mode = Options::ALPHA;
        else if (val == "reverse")
            opts.sort_mode = Options::REVERSE;
        else
            throw std::runtime_error("Invalid value for --row-order");
    }
    opts.use_syslog = parser.has_flag("--syslog") || cfg_flag("--syslog");
    if (parser.has_flag("--syslog-facility") || cfg_opts.count("--syslog-facility")) {
        std::string val = parser.get_option("--syslog-facility");
        if (val.empty())
            val = cfg_opt("--syslog-facility");
        bool ok2 = false;
        int fac = parse_int(val, 0, INT_MAX, ok2);
        if (!ok2)
            throw std::runtime_error("Invalid value for --syslog-facility");
        opts.syslog_facility = fac;
    }
    if (parser.has_flag("--pull-timeout") || cfg_opts.count("--pull-timeout")) {
        std::string val = parser.get_option("--pull-timeout");
        if (val.empty())
            val = cfg_opt("--pull-timeout");
        bool ok2 = false;
        int sec = parse_int(val, 1, INT_MAX, ok2);
        if (!ok2)
            throw std::runtime_error("Invalid value for --pull-timeout");
        opts.pull_timeout = std::chrono::seconds(sec);
    }
    opts.skip_timeout =
        !(parser.has_flag("--dont-skip-timeouts") || cfg_flag("--dont-skip-timeouts"));
    opts.skip_accessible_errors =
        parser.has_flag("--skip-accessible-errors") || cfg_flag("--skip-accessible-errors");
    opts.exit_on_timeout = parser.has_flag("--exit-on-timeout") || cfg_flag("--exit-on-timeout");
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
    if (opts.root.empty() && !opts.show_help && !opts.print_version && !opts.show_service &&
        ((opts.attach_name.empty() && !opts.reattach) || opts.run_background || persist_flag))
        throw std::runtime_error("Root path required");
    if (persist_flag) {
        opts.persist = true;
        std::string name = persist_val;
        if (name.empty())
            name = opts.root.filename().string();
        if (opts.attach_name.empty())
            opts.attach_name = name;
    }
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
        if (values.count("--max-runtime")) {
            int sec = parse_int(ropt("--max-runtime"), 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo max-runtime");
            ro.max_runtime = std::chrono::seconds(sec);
        }
        if (values.count("--pull-timeout")) {
            int sec = parse_int(ropt("--pull-timeout"), 1, INT_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo pull-timeout");
            ro.pull_timeout = std::chrono::seconds(sec);
        }
        opts.repo_overrides[fs::path(repo)] = ro;
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
    return opts;
}

bool alerts_allowed(const Options& opts) {
    if (opts.confirm_alert || opts.sudo_su)
        return true;
    if (opts.interval < 15 || opts.force_pull)
        return false;
    return true;
}
