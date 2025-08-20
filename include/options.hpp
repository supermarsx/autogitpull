#ifndef OPTIONS_HPP
#define OPTIONS_HPP
#include <filesystem>
#include <vector>
#include <string>
#include <chrono>
#include <map>
#include <functional>
#include "logger.hpp"
#include "repo_options.hpp"
#include "tui.hpp"

struct LoggingOptions {
    LogLevel log_level = LogLevel::INFO;
    std::filesystem::path log_dir;
    std::string log_file;
    size_t max_log_size = 0;
    bool json_log = false;
    bool use_syslog = false;
    int syslog_facility = 0;
};

struct ServiceOptions {
    bool install_daemon = false;
    bool uninstall_daemon = false;
    std::string daemon_config;
    bool install_service = false;
    bool uninstall_service = false;
    bool start_service = false;
    bool stop_service = false;
    bool force_stop_service = false;
    bool restart_service = false;
    bool force_restart_service = false;
    std::string service_config;
    std::string service_name = "autogitpull";
    std::string daemon_name = "autogitpull";
    std::string start_service_name;
    std::string stop_service_name;
    std::string restart_service_name;
    std::string start_daemon_name;
    std::string stop_daemon_name;
    std::string restart_daemon_name;
    bool start_daemon = false;
    bool stop_daemon = false;
    bool force_stop_daemon = false;
    bool restart_daemon = false;
    bool force_restart_daemon = false;
    bool service_status = false;
    bool daemon_status = false;
    bool show_service = false;
    bool run_background = false;
    bool reattach = false;
    std::string attach_name;
    bool persist = false;
    int respawn_max = 0;
    std::chrono::minutes respawn_window{10};
    std::chrono::milliseconds respawn_delay{1000};
    bool kill_all = false;
    bool kill_on_sleep = false;
    bool list_instances = false;
    bool list_services = false;
};

struct ResourceLimits {
    unsigned int cpu_poll_sec = 5;
    unsigned int mem_poll_sec = 5;
    unsigned int thread_poll_sec = 5;
    size_t concurrency = 1;
    size_t max_threads = 0;
    double cpu_percent_limit = 0.0;
    unsigned long long cpu_core_mask = 0;
    size_t mem_limit = 0;
    size_t download_limit = 0;
    size_t upload_limit = 0;
    size_t disk_limit = 0;
    size_t total_traffic_limit = 0;
    std::chrono::seconds pull_timeout{0};
    bool skip_timeout = true;
    bool exit_on_timeout = false;
};

struct Options {
    std::filesystem::path root;
    std::string remote_name = "origin";
    bool include_private = false;
    bool show_skipped = false;
    bool show_notgit = false;
    bool show_version = false;
    bool remove_lock = false;
    bool ignore_lock = false;
    bool cli = false;
    bool silent = false;
    bool recursive_scan = false;
    bool check_only = false;
    bool hash_check = true;
    bool force_pull = false;
    LoggingOptions logging;
    std::filesystem::path ssh_public_key;
    std::filesystem::path ssh_private_key;
    std::filesystem::path credential_file;
    int interval = 30;
    std::chrono::milliseconds refresh_ms{250};
    ResourceLimits limits;
    size_t max_depth = 0;
    bool cpu_tracker = true;
    bool mem_tracker = true;
    bool thread_tracker = true;
    bool net_tracker = false;
    bool show_vmem = false;
    bool show_commit_date = false;
    bool show_commit_author = false;
    bool show_pull_author = false;
    bool show_repo_count = false;
    bool censor_names = false;
    char censor_char = '*';
    bool session_dates_only = false;
    bool show_datetime_line = true;
    bool show_header = true;
    bool no_colors = false;
    std::string custom_color;
    std::string theme_file;
    TuiTheme theme;
    std::vector<std::filesystem::path> include_dirs;
    std::vector<std::filesystem::path> ignore_dirs;
    bool enable_history = false;
    std::string history_file = ".autogitpull.config";
    bool enable_hotkeys = false;
    bool auto_config = false;
    bool auto_reload_config = false;
    std::filesystem::path config_file;
    std::vector<std::string> original_args;
    bool rerun_last = false;
    bool save_args = false;
    bool debug_memory = false;
    bool dump_state = false;
    size_t dump_threshold = 0;
    bool single_run = false;
    bool single_repo = false;
    ServiceOptions service;
    bool show_runtime = false;
    std::chrono::seconds runtime_limit{0};
    bool rescan_new = false;
    std::chrono::minutes rescan_interval{5};
    std::chrono::seconds updated_since{0};
    bool keep_first_valid = false;
    bool wait_empty = false;
    int wait_empty_limit = 0;
    bool skip_accessible_errors = false;
    bool skip_unavailable = true;
    bool retry_skipped = false;
    bool reset_skipped = false;
    bool cli_print_skipped = false;
    bool show_help = false;
    bool print_version = false;
    bool hard_reset = false;
    bool confirm_reset = false;
    bool confirm_alert = false;
    bool sudo_su = false;
    bool mutant_mode = false;
    bool confirm_mutant = false;
    bool recover_mutant = false;
    std::filesystem::path mutant_config;
    bool add_ignore = false;
    bool remove_ignore = false;
    bool clear_ignores = false;
    bool find_ignores = false;
    std::string add_ignore_repo;
    std::string remove_ignore_repo;
    unsigned int depth = 2;
    std::map<std::filesystem::path, RepoOptions> repo_settings;
    enum SortMode { UPDATED, ALPHA, REVERSE } sort_mode = UPDATED;
};

struct LegacyOptions : LoggingOptions, ServiceOptions, ResourceLimits {
    LegacyOptions(const LoggingOptions& l, const ServiceOptions& s, const ResourceLimits& r)
        : LoggingOptions(l), ServiceOptions(s), ResourceLimits(r) {}
};

inline LegacyOptions aggregate_options(const Options& opts) {
    return LegacyOptions(opts.logging, opts.service, opts.limits);
}

/**
 * Parse command-line arguments and configuration files to populate an Options
 * instance.
 *
 * @param argc Number of command-line arguments.
 * @param argv Argument vector.
 * @return Fully populated Options structure.
 * Side effects on Options: none. The resulting Options is returned by value
 * and the original arguments remain untouched.
 */
Options parse_options(int argc, char* argv[]);

/**
 * Determine whether user-visible alerts are permitted.
 *
 * @param opts Configuration values to inspect.
 * @return true if alerts may be shown, false otherwise.
 * Side effects on Options: none; opts is read-only.
 */
bool alerts_allowed(const Options& opts);

/**
 * Execute the main application event loop.
 *
 * @param opts Options controlling application behavior. Passed by value so
 *             modifications are local to the event loop.
 * @return Process exit code to report to the caller.
 * Side effects on Options: none to the caller's instance since it is copied
 * on entry.
 */
int run_event_loop(Options opts);

class ArgParser;

/**
 * Parse service-related flags and options.
 *
 * @param opts    Options structure to update with service settings.
 * @param parser  Command-line parser containing service arguments.
 * @param cfg_flag Function to query boolean service flags from config.
 * @param cfg_opt  Function to fetch service option values from config.
 * @param cfg_opts Map of service option names to their string values.
 * @return void
 * Side effects on Options: updates opts.service fields based on CLI and
 * configuration data.
 */
void parse_service_options(Options& opts, ArgParser& parser,
                           const std::function<bool(const std::string&)>& cfg_flag,
                           const std::function<std::string(const std::string&)>& cfg_opt,
                           const std::map<std::string, std::string>& cfg_opts);

/**
 * Parse tracker-related flags to enable or disable resource trackers.
 *
 * @param opts    Options structure to update with tracker settings.
 * @param parser  Command-line parser containing tracker arguments.
 * @param cfg_flag Function to query tracker flags from config.
 * @return void
 * Side effects on Options: updates tracker fields within opts based on
 * provided inputs.
 */
void parse_tracker_options(Options& opts, ArgParser& parser,
                           const std::function<bool(const std::string&)>& cfg_flag);

/**
 * Parse resource limit settings from command-line and configuration.
 *
 * @param opts    Options structure to update with limit settings.
 * @param parser  Command-line parser containing limit arguments.
 * @param cfg_opt Function to fetch limit option values from config.
 * @param cfg_opts Map of limit option names to their string values.
 * @return void
 * Side effects on Options: updates opts.limits to reflect configured
 * resource limits.
 */
void parse_limits(Options& opts, ArgParser& parser,
                  const std::function<std::string(const std::string&)>& cfg_opt,
                  const std::map<std::string, std::string>& cfg_opts);

#endif // OPTIONS_HPP
