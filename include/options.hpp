#ifndef OPTIONS_HPP
#define OPTIONS_HPP
#include <filesystem>
#include <vector>
#include <string>
#include <chrono>
#include "logger.hpp"

struct Options {
    std::filesystem::path root;
    bool include_private = false;
    bool show_skipped = false;
    bool show_version = false;
    bool remove_lock = false;
    bool cli = false;
    bool silent = false;
    bool recursive_scan = false;
    bool check_only = false;
    bool hash_check = true;
    bool force_pull = false;
    LogLevel log_level = LogLevel::INFO;
    std::filesystem::path log_dir;
    std::string log_file;
    int interval = 30;
    std::chrono::milliseconds refresh_ms{250};
    unsigned int cpu_poll_sec = 5;
    unsigned int mem_poll_sec = 5;
    unsigned int thread_poll_sec = 5;
    size_t concurrency = 1;
    size_t max_threads = 0;
    int cpu_percent_limit = 0;
    unsigned long long cpu_core_mask = 0;
    size_t mem_limit = 0;
    size_t download_limit = 0;
    size_t upload_limit = 0;
    size_t disk_limit = 0;
    size_t max_depth = 0;
    bool cpu_tracker = true;
    bool mem_tracker = true;
    bool thread_tracker = true;
    bool net_tracker = false;
    bool show_vmem = false;
    bool show_commit_date = false;
    bool show_commit_author = false;
    bool show_datetime_line = true;
    bool show_header = true;
    bool show_repo_count = false;
    bool no_colors = false;
    std::string custom_color;
    std::vector<std::filesystem::path> ignore_dirs;
    bool debug_memory = false;
    bool dump_state = false;
    size_t dump_threshold = 0;
    bool single_run = false;
    bool single_repo = false;
    bool install_daemon = false;
    bool uninstall_daemon = false;
    std::string daemon_config;
    bool install_service = false;
    bool uninstall_service = false;
    std::string service_config;
    bool run_background = false;
    bool reattach = false;
    std::string attach_name;
    bool show_runtime = false;
    std::chrono::seconds runtime_limit{0};
    bool persist = false;
    int respawn_max = 0;
    std::chrono::minutes respawn_window{10};
    bool kill_all = false;
    bool rescan_new = false;
    std::chrono::minutes rescan_interval{5};
    bool use_syslog = false;
    int syslog_facility = 0;
    std::chrono::seconds pull_timeout{0};
    bool skip_timeout = true;
    bool show_help = false;
    bool print_version = false;
    enum SortMode { DEFAULT, ALPHA, REVERSE } sort_mode = DEFAULT;
};

Options parse_options(int argc, char* argv[]);
int run_event_loop(const Options& opts);

#endif // OPTIONS_HPP
