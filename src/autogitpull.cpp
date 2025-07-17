#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <set>
#include <map>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <sstream>
#include <cctype>
#include <csignal>
#include <memory>
#include "arg_parser.hpp"
#include "git_utils.hpp"
#include "tui.hpp"
#include "repo.hpp"
#include "logger.hpp"
#include "time_utils.hpp"
#include "resource_utils.hpp"
#include "system_utils.hpp"
#include "version.hpp"
#include "thread_utils.hpp"
#include "config_utils.hpp"
#include "debug_utils.hpp"

namespace fs = std::filesystem;

struct AltScreenGuard {
    AltScreenGuard() {
        enable_win_ansi();
        std::cout << "\033[?1049h";
    }
    ~AltScreenGuard() { std::cout << "\033[?1049l" << std::flush; }
};

std::atomic<bool>* g_running_ptr = nullptr;
bool debugMemory = false;
bool dumpState = false;
size_t dumpThreshold = 0;

void handle_signal(int) {
    if (g_running_ptr)
        g_running_ptr->store(false);
}

std::string status_label(RepoStatus status) {
    switch (status) {
    case RS_PENDING:
        return "Pending";
    case RS_CHECKING:
        return "Checking";
    case RS_UP_TO_DATE:
        return "UpToDate";
    case RS_PULLING:
        return "Pulling";
    case RS_PULL_OK:
        return "Pulled";
    case RS_PKGLOCK_FIXED:
        return "PkgLockOk";
    case RS_ERROR:
        return "Error";
    case RS_SKIPPED:
        return "Skipped";
    case RS_HEAD_PROBLEM:
        return "HEAD/BR";
    case RS_DIRTY:
        return "Dirty";
    case RS_REMOTE_AHEAD:
        return "RemoteUp";
    }
    return "";
}

/**
 * @brief Collect repository paths under @a root.
 *
 * @param root      Directory to scan.
 * @param recursive Whether to scan recursively.
 * @param ignore    List of directories to skip.
 * @return Vector of discovered directories.
 */
std::vector<fs::path> build_repo_list(const fs::path& root, bool recursive,
                                      const std::vector<fs::path>& ignore, size_t max_depth) {
    std::vector<fs::path> result;
    if (recursive) {
        for (fs::recursive_directory_iterator it(root), end; it != end; ++it) {
            if (max_depth > 0 && it.depth() >= static_cast<int>(max_depth)) {
                it.disable_recursion_pending();
                continue;
            }
            if (!it->is_directory())
                continue;
            fs::path p = it->path();
            if (std::find(ignore.begin(), ignore.end(), p) != ignore.end()) {
                it.disable_recursion_pending();
                continue;
            }
            result.push_back(p);
        }
    } else {
        for (const auto& entry : fs::directory_iterator(root)) {
            fs::path p = entry.path();
            if (std::find(ignore.begin(), ignore.end(), p) != ignore.end())
                continue;
            result.push_back(p);
        }
    }
    return result;
}

/** Print the command line help text. */
void print_help(const char* prog) {
    std::cout << "Usage: " << prog << " <root-folder> [options]\n\n"
              << "Options:\n"
              << "  -p, --include-private   Include private repositories\n"
              << "  -k, --show-skipped      Show skipped repositories\n"
              << "  -v, --show-version      Display program version in TUI\n"
              << "  -V, --version           Print program version and exit\n"
              << "  -i, --interval <sec>    Delay between scans\n"
              << "  -r, --refresh-rate <ms> TUI refresh rate\n"
              << "  -d, --log-dir <path>    Directory for pull logs\n"
              << "  -l, --log-file <path>   File for general logs\n"
              << "  -y, --config-yaml <f>  Load options from YAML file\n"
              << "  -j, --config-json <f>  Load options from JSON file\n"
              << "      --ignore <dir>      Directory to ignore (repeatable)\n"
              << "      --disk-limit <KB/s> Limit disk throughput\n"
              << "      --recursive         Scan subdirectories recursively\n"
              << "  -D, --max-depth <n>    Limit recursive scan depth\n"
              << "  -c, --cli               Use console output\n"
              << "  -s, --silent            Disable console output\n"
              << "      --debug-memory      Log memory usage each scan\n"
              << "      --dump-state        Dump container state when large\n"
              << "      --dump-large <n>    Dump threshold for --dump-state\n"
              << "  -h, --help              Show this message\n";
}

void draw_cli(const std::vector<fs::path>& all_repos,
              const std::map<fs::path, RepoInfo>& repo_infos, int seconds_left, bool scanning,
              const std::string& action, bool show_skipped) {
    std::cout << "Status: ";
    if (scanning)
        std::cout << action;
    else
        std::cout << "Idle";
    std::cout << " - Next scan in " << seconds_left << "s\n";
    for (const auto& p : all_repos) {
        RepoInfo ri;
        auto it = repo_infos.find(p);
        if (it != repo_infos.end())
            ri = it->second;
        else
            ri = RepoInfo{p, RS_PENDING, "Pending...", "", "", "", 0, false};
        if (ri.status == RS_SKIPPED && !show_skipped)
            continue;
        std::cout << " [" << status_label(ri.status) << "] " << p.filename().string();
        if (!ri.branch.empty()) {
            std::cout << " (" << ri.branch;
            if (!ri.commit.empty())
                std::cout << "@" << ri.commit;
            std::cout << ")";
        }
        if (!ri.message.empty())
            std::cout << " - " << ri.message;
        if (ri.status == RS_PULLING)
            std::cout << " (" << ri.progress << "%)";
        if (ri.auth_failed)
            std::cout << " [AUTH]";
        std::cout << "\n";
    }
    std::cout << std::flush;
}

// Process a single repository
void process_repo(const fs::path& p, std::map<fs::path, RepoInfo>& repo_infos,
                  std::set<fs::path>& skip_repos, std::mutex& mtx, std::atomic<bool>& running,
                  std::string& action, std::mutex& action_mtx, bool include_private,
                  const fs::path& log_dir, bool check_only, bool hash_check, size_t down_limit,
                  size_t up_limit, size_t disk_limit, bool silent, bool force_pull) {
    if (!running)
        return;
    if (logger_initialized())
        log_debug("Checking repo " + p.string());
    {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = repo_infos.find(p);
        if (it != repo_infos.end() &&
            (it->second.status == RS_PULLING || it->second.status == RS_CHECKING)) {
            // Repo already being processed elsewhere
            if (!silent)
                std::cerr << "Skipping " << p << " - busy\n";
            if (logger_initialized())
                log_debug("Skipping " + p.string() + " - busy");
            return;
        }
    }
    RepoInfo ri;
    ri.path = p;
    ri.auth_failed = false;
    {
        std::lock_guard<std::mutex> lk(action_mtx);
        action = "Checking " + p.filename().string();
    }
    if (!fs::exists(p)) {
        ri.status = RS_ERROR;
        ri.message = "Missing";
        if (logger_initialized())
            log_error(p.string() + " missing");
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
        return;
    }
    if (skip_repos.count(p)) {
        ri.status = RS_SKIPPED;
        ri.message = "Skipped after fatal error";
        if (logger_initialized())
            log_warning(p.string() + " skipped after fatal error");
        std::lock_guard<std::mutex> lk(mtx);
        repo_infos[p] = ri;
        return;
    }
    try {
        ri.status = RS_CHECKING;
        ri.message = "";
        if (fs::is_directory(p) && git::is_git_repo(p)) {
            ri.commit = git::get_local_hash(p);
            if (ri.commit.size() > 7)
                ri.commit = ri.commit.substr(0, 7);
            std::string origin = git::get_origin_url(p);
            if (!include_private) {
                if (!git::is_github_url(origin)) {
                    ri.status = RS_SKIPPED;
                    ri.message = "Non-GitHub repo (skipped)";
                    if (logger_initialized())
                        log_debug(p.string() + " skipped: non-GitHub repo");
                    std::lock_guard<std::mutex> lk(mtx);
                    repo_infos[p] = ri;
                    return;
                }
                if (!git::remote_accessible(p)) {
                    ri.status = RS_SKIPPED;
                    ri.message = "Private or inaccessible repo";
                    if (logger_initialized())
                        log_debug(p.string() + " skipped: private or inaccessible");
                    std::lock_guard<std::mutex> lk(mtx);
                    repo_infos[p] = ri;
                    return;
                }
            }
            ri.branch = git::get_current_branch(p);
            if (ri.branch.empty() || ri.branch == "HEAD") {
                ri.status = RS_HEAD_PROBLEM;
                ri.message = "Detached HEAD or branch error";
                skip_repos.insert(p);
            } else {
                bool needs_pull = true;
                if (hash_check) {
                    std::string local = git::get_local_hash(p);
                    bool auth_fail = false;
                    std::string remote =
                        git::get_remote_hash(p, ri.branch, include_private, &auth_fail);
                    ri.auth_failed = auth_fail;
                    if (local.empty() || remote.empty()) {
                        ri.status = RS_ERROR;
                        ri.message = "Error getting hashes or remote";
                        skip_repos.insert(p);
                        needs_pull = false;
                    } else if (local == remote) {
                        ri.status = RS_UP_TO_DATE;
                        ri.message = "Up to date";
                        ri.commit = local.substr(0, 7);
                        needs_pull = false;
                    }
                }
                if (needs_pull) {
                    if (check_only) {
                        ri.status = RS_REMOTE_AHEAD;
                        ri.message = hash_check ? "Remote ahead" : "Update possible";
                        ri.commit = git::get_local_hash(p);
                        if (ri.commit.size() > 7)
                            ri.commit = ri.commit.substr(0, 7);
                        if (logger_initialized())
                            log_debug(p.string() + " remote ahead");
                    } else {
                        ri.status = RS_PULLING;
                        ri.message = "Remote ahead, pulling...";
                        ri.progress = 0;
                        {
                            std::lock_guard<std::mutex> lk(action_mtx);
                            action = "Pulling " + p.filename().string();
                        }
                        {
                            std::lock_guard<std::mutex> lk(mtx);
                            repo_infos[p] = ri;
                        }
                        std::string pull_log;
                        std::function<void(int)> progress_cb = [&](int pct) {
                            std::lock_guard<std::mutex> lk(mtx);
                            repo_infos[p].progress = pct;
                        };
                        bool pull_auth_fail = false;
                        int code = git::try_pull(p, pull_log, &progress_cb, include_private,
                                                 &pull_auth_fail, down_limit, up_limit, disk_limit,
                                                 force_pull);
                        ri.auth_failed = pull_auth_fail;
                        ri.last_pull_log = pull_log;
                        fs::path log_file_path;
                        if (!log_dir.empty()) {
                            std::string ts = timestamp();
                            for (char& ch : ts) {
                                if (ch == ' ' || ch == ':')
                                    ch = '_';
                                else if (ch == '/')
                                    ch = '-';
                            }
                            log_file_path = log_dir / (p.filename().string() + "_" + ts + ".log");
                            std::ofstream ofs(log_file_path);
                            ofs << pull_log;
                        }
                        if (code == 0) {
                            ri.status = RS_PULL_OK;
                            ri.message = "Pulled successfully";
                            ri.commit = git::get_local_hash(p);
                            if (ri.commit.size() > 7)
                                ri.commit = ri.commit.substr(0, 7);
                            if (logger_initialized())
                                log_info(p.string() + " pulled successfully");
                        } else if (code == 1) {
                            ri.status = RS_PKGLOCK_FIXED;
                            ri.message = "package-lock.json auto-reset & pulled";
                            ri.commit = git::get_local_hash(p);
                            if (ri.commit.size() > 7)
                                ri.commit = ri.commit.substr(0, 7);
                            if (logger_initialized())
                                log_info(p.string() + " package-lock reset and pulled");
                        } else if (code == 3) {
                            ri.status = RS_DIRTY;
                            ri.message = "Local changes present";
                        } else {
                            ri.status = RS_ERROR;
                            ri.message = "Pull failed (see log)";
                            skip_repos.insert(p);
                            if (logger_initialized())
                                log_error(p.string() + " pull failed");
                        }
                        if (!log_file_path.empty()) {
                            ri.message += " - " + log_file_path.string();
                        }
                    }
                }
            }
        } else {
            ri.status = RS_SKIPPED;
            ri.message = "Not a git repo (skipped)";
            skip_repos.insert(p);
            if (logger_initialized())
                log_debug(p.string() + " skipped: not a git repo");
        }
    } catch (const fs::filesystem_error& e) {
        ri.status = RS_ERROR;
        ri.message = e.what();
        skip_repos.insert(p);
        if (logger_initialized())
            log_error(p.string() + " error: " + ri.message);
    }
    std::lock_guard<std::mutex> lk(mtx);
    repo_infos[p] = ri;
    if (logger_initialized())
        log_debug(p.string() + " -> " + ri.message);
}

// Background thread: scan and update info
void scan_repos(const std::vector<fs::path>& all_repos, std::map<fs::path, RepoInfo>& repo_infos,
                std::set<fs::path>& skip_repos, std::mutex& mtx, std::atomic<bool>& scanning_flag,
                std::atomic<bool>& running, std::string& action, std::mutex& action_mtx,
                bool include_private, const fs::path& log_dir, bool check_only, bool hash_check,
                size_t concurrency, int cpu_percent_limit, size_t mem_limit, size_t down_limit,
                size_t up_limit, size_t disk_limit, bool silent, bool force_pull) {
    git::GitInitGuard guard;
    size_t mem_before = procutil::get_memory_usage_mb();
    size_t virt_before = procutil::get_virtual_memory_kb();

    {
        std::lock_guard<std::mutex> lk(mtx);
        // Retain repo_infos so statuses persist between scans
        std::set<fs::path> empty_set;
        skip_repos.swap(empty_set);
    }

    if (concurrency == 0)
        concurrency = 1;
    concurrency = std::min(concurrency, all_repos.size());
    if (logger_initialized())
        log_debug("Scanning repositories");

    std::atomic<size_t> next_index{0};
    auto worker = [&]() {
        while (running) {
            size_t idx = next_index.fetch_add(1);
            if (idx >= all_repos.size())
                break;
            const auto& p = all_repos[idx];
            process_repo(p, repo_infos, skip_repos, mtx, running, action, action_mtx,
                         include_private, log_dir, check_only, hash_check, down_limit, up_limit,
                         disk_limit, silent, force_pull);
            if (mem_limit > 0 && procutil::get_memory_usage_mb() > mem_limit) {
                log_error("Memory limit exceeded");
                running = false;
                break;
            }
            if (cpu_percent_limit > 0) {
                double cpu = procutil::get_cpu_percent();
                if (cpu > cpu_percent_limit) {
                    double over = cpu / cpu_percent_limit - 1.0;
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(static_cast<int>(over * 100)));
                }
            }
        }
    };

    std::vector<std::jthread> threads;
    threads.reserve(concurrency);
    const size_t max_threads = concurrency;
    for (size_t i = 0; i < max_threads; ++i) {
        if (threads.size() >= max_threads)
            break;
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }
    threads.clear();
    threads.shrink_to_fit();
    if (debugMemory) {
        size_t mem_after = procutil::get_memory_usage_mb();
        size_t virt_after = procutil::get_virtual_memory_kb();
        log_debug("Memory before=" + std::to_string(mem_before) + "MB after=" +
                  std::to_string(mem_after) + "MB delta=" + std::to_string(mem_after - mem_before) +
                  "MB vmem_before=" + std::to_string(virt_before / 1024) +
                  "MB vmem_after=" + std::to_string(virt_after / 1024) +
                  "MB vmem_delta=" + std::to_string((virt_after - virt_before) / 1024) + "MB");
        debug_utils::log_container_size("repo_infos", repo_infos);
        debug_utils::log_container_size("skip_repos", skip_repos);
        if (dumpState && repo_infos.size() > dumpThreshold)
            debug_utils::dump_repo_infos(repo_infos);
        if (dumpState && skip_repos.size() > dumpThreshold)
            debug_utils::dump_container("skip_repos", skip_repos);
    }
    scanning_flag = false;
    {
        std::lock_guard<std::mutex> lk(action_mtx);
        action = "Idle";
    }
    if (logger_initialized())
        log_debug("Scan complete");
}

#ifndef AUTOGITPULL_NO_MAIN
int main(int argc, char* argv[]) {
    git::GitInitGuard git_guard;
    try {
        // Parse config file option first
        const std::set<std::string> pre_known{"--config-yaml", "--config-json"};
        const std::map<char, std::string> pre_short{{'y', "--config-yaml"}, {'j', "--config-json"}};
        ArgParser pre_parser(argc, argv, pre_known, pre_short);
        std::map<std::string, std::string> cfg_opts;
        if (pre_parser.has_flag("--config-yaml")) {
            std::string cfg = pre_parser.get_option("--config-yaml");
            if (cfg.empty()) {
                std::cerr << "--config-yaml requires a file\n";
                return 1;
            }
            std::string err;
            if (!load_yaml_config(cfg, cfg_opts, err)) {
                std::cerr << "Failed to load config: " << err << "\n";
                return 1;
            }
        }
        if (pre_parser.has_flag("--config-json")) {
            std::string cfg = pre_parser.get_option("--config-json");
            if (cfg.empty()) {
                std::cerr << "--config-json requires a file\n";
                return 1;
            }
            std::string err;
            if (!load_json_config(cfg, cfg_opts, err)) {
                std::cerr << "Failed to load config: " << err << "\n";
                return 1;
            }
        }

        const std::set<std::string> known{
            "--include-private", "--show-skipped",   "--show-version",      "--version",
            "--interval",        "--refresh-rate",   "--cpu-poll",          "--mem-poll",
            "--thread-poll",     "--log-dir",        "--log-file",          "--concurrency",
            "--check-only",      "--no-hash-check",  "--log-level",         "--verbose",
            "--max-threads",     "--cpu-percent",    "--cpu-cores",         "--mem-limit",
            "--no-cpu-tracker",  "--no-mem-tracker", "--no-thread-tracker", "--help",
            "--threads",         "--single-thread",  "--net-tracker",       "--download-limit",
            "--upload-limit",    "--disk-limit",     "--max-depth",         "--cli",
            "--silent",          "--recursive",      "--config-yaml",       "--config-json",
            "--ignore",          "--force-pull",     "--discard-dirty",     "--debug-memory",
            "--dump-state",      "--dump-large"};
        const std::map<char, std::string> short_opts{
            {'p', "--include-private"}, {'k', "--show-skipped"}, {'v', "--show-version"},
            {'V', "--version"},         {'i', "--interval"},     {'r', "--refresh-rate"},
            {'d', "--log-dir"},         {'l', "--log-file"},     {'y', "--config-yaml"},
            {'j', "--config-json"},     {'c', "--cli"},          {'s', "--silent"},
            {'D', "--max-depth"},       {'h', "--help"}};
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

        bool cli = parser.has_flag("--cli") || cfg_flag("--cli");
        bool silent = parser.has_flag("--silent") || cfg_flag("--silent");
        bool recursive_scan = parser.has_flag("--recursive") || cfg_flag("--recursive");

        if (parser.has_flag("--version")) {
            std::cout << AUTOGITPULL_VERSION << "\n";
            return 0;
        }

        if (parser.has_flag("--help")) {
            print_help(argv[0]);
            return 0;
        }

        if (parser.positional().size() != 1) {
            if (!silent)
                print_help(argv[0]);
            return 1;
        }

        if (!parser.unknown_flags().empty()) {
            for (const auto& f : parser.unknown_flags()) {
                if (!silent)
                    std::cerr << "Unknown option: " << f << "\n";
            }
            return 1;
        }
        bool include_private =
            parser.has_flag("--include-private") || cfg_flag("--include-private");
        bool show_skipped = parser.has_flag("--show-skipped") || cfg_flag("--show-skipped");
        bool show_version = parser.has_flag("--show-version") || cfg_flag("--show-version");
        bool check_only = parser.has_flag("--check-only") || cfg_flag("--check-only");
        bool hash_check = !(parser.has_flag("--no-hash-check") || cfg_flag("--no-hash-check"));
        bool force_pull = parser.has_flag("--force-pull") || parser.has_flag("--discard-dirty") ||
                          cfg_flag("--force-pull") || cfg_flag("--discard-dirty");
        LogLevel log_level = LogLevel::INFO;
        if (parser.has_flag("--verbose") || cfg_flag("--verbose")) {
            log_level = LogLevel::DEBUG;
        }
        if (parser.has_flag("--log-level") || cfg_opts.count("--log-level")) {
            std::string val = parser.get_option("--log-level");
            if (val.empty())
                val = cfg_opt("--log-level");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--log-level requires a value\n";
                return 1;
            }
            for (auto& c : val)
                c = toupper(static_cast<unsigned char>(c));
            if (val == "DEBUG")
                log_level = LogLevel::DEBUG;
            else if (val == "INFO")
                log_level = LogLevel::INFO;
            else if (val == "WARNING" || val == "WARN")
                log_level = LogLevel::WARNING;
            else if (val == "ERROR")
                log_level = LogLevel::ERROR;
            else {
                if (!silent)
                    std::cerr << "Invalid log level: " << val << "\n";
                return 1;
            }
        }
        size_t concurrency = std::thread::hardware_concurrency();
        if (concurrency == 0)
            concurrency = 1;
        size_t max_threads = 0;
        int cpu_percent_limit = 0;
        unsigned long long cpu_core_mask = 0;
        size_t mem_limit = 0;
        size_t down_limit = 0;
        size_t up_limit = 0;
        size_t disk_limit = 0;
        size_t max_depth = 0;
        bool cpu_tracker = true;
        bool mem_tracker = true;
        bool thread_tracker = true;
        bool net_tracker = false;
        int interval = 30;
        std::chrono::milliseconds refresh_ms(250);
        unsigned int cpu_poll_sec = 5;
        unsigned int mem_poll_sec = 5;
        unsigned int thread_poll_sec = 5;

        // Apply YAML defaults
        if (cfg_opts.count("--interval")) {
            try {
                interval = std::stoi(cfg_opt("--interval"));
            } catch (...) {
                std::cerr << "Invalid value in YAML for interval\n";
                return 1;
            }
        }
        if (cfg_opts.count("--refresh-rate")) {
            try {
                refresh_ms = std::chrono::milliseconds(std::stoi(cfg_opt("--refresh-rate")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for refresh-rate\n";
                return 1;
            }
        }
        if (cfg_opts.count("--cpu-poll")) {
            try {
                cpu_poll_sec = static_cast<unsigned int>(std::stoul(cfg_opt("--cpu-poll")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for cpu-poll\n";
                return 1;
            }
        }
        if (cfg_opts.count("--mem-poll")) {
            try {
                mem_poll_sec = static_cast<unsigned int>(std::stoul(cfg_opt("--mem-poll")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for mem-poll\n";
                return 1;
            }
        }
        if (cfg_opts.count("--thread-poll")) {
            try {
                thread_poll_sec = static_cast<unsigned int>(std::stoul(cfg_opt("--thread-poll")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for thread-poll\n";
                return 1;
            }
        }
        if (cfg_opts.count("--threads")) {
            try {
                concurrency = static_cast<size_t>(std::stoul(cfg_opt("--threads")));
                if (concurrency == 0)
                    concurrency = 1;
            } catch (...) {
                std::cerr << "Invalid value in YAML for threads\n";
                return 1;
            }
        }
        if (cfg_flag("--single-thread"))
            concurrency = 1;
        if (cfg_opts.count("--concurrency")) {
            try {
                concurrency = static_cast<size_t>(std::stoul(cfg_opt("--concurrency")));
                if (concurrency == 0)
                    concurrency = 1;
            } catch (...) {
                std::cerr << "Invalid value in YAML for concurrency\n";
                return 1;
            }
        }
        if (cfg_opts.count("--max-threads")) {
            try {
                max_threads = static_cast<size_t>(std::stoul(cfg_opt("--max-threads")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for max-threads\n";
                return 1;
            }
        }
        if (cfg_opts.count("--cpu-percent")) {
            std::string val = cfg_opt("--cpu-percent");
            if (!val.empty() && val.back() == '%')
                val.pop_back();
            try {
                cpu_percent_limit = std::stoi(val);
            } catch (...) {
                std::cerr << "Invalid value in YAML for cpu-percent\n";
                return 1;
            }
        }
        if (cfg_opts.count("--cpu-cores")) {
            try {
                cpu_core_mask = std::stoull(cfg_opt("--cpu-cores"), nullptr, 0);
            } catch (...) {
                std::cerr << "Invalid value in YAML for cpu-cores\n";
                return 1;
            }
        }
        if (cfg_opts.count("--mem-limit")) {
            try {
                mem_limit = static_cast<size_t>(std::stoul(cfg_opt("--mem-limit")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for mem-limit\n";
                return 1;
            }
        }
        if (cfg_opts.count("--download-limit")) {
            try {
                down_limit = static_cast<size_t>(std::stoul(cfg_opt("--download-limit")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for download-limit\n";
                return 1;
            }
        }
        if (cfg_opts.count("--upload-limit")) {
            try {
                up_limit = static_cast<size_t>(std::stoul(cfg_opt("--upload-limit")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for upload-limit\n";
                return 1;
            }
        }
        if (cfg_opts.count("--disk-limit")) {
            try {
                disk_limit = static_cast<size_t>(std::stoul(cfg_opt("--disk-limit")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for disk-limit\n";
                return 1;
            }
        }
        if (cfg_opts.count("--max-depth")) {
            try {
                max_depth = static_cast<size_t>(std::stoul(cfg_opt("--max-depth")));
            } catch (...) {
                std::cerr << "Invalid value in YAML for max-depth\n";
                return 1;
            }
        }
        cpu_tracker = !cfg_flag("--no-cpu-tracker");
        mem_tracker = !cfg_flag("--no-mem-tracker");
        thread_tracker = !cfg_flag("--no-thread-tracker");
        net_tracker = cfg_flag("--net-tracker");
        debugMemory = cfg_flag("--debug-memory");
        dumpState = cfg_flag("--dump-state");
        if (cfg_opts.count("--dump-large")) {
            try {
                dumpThreshold = static_cast<size_t>(std::stoul(cfg_opt("--dump-large")));
            } catch (...) {
                std::cerr << "Invalid value in config for dump-large\n";
                return 1;
            }
        }

        if (parser.has_flag("--interval")) {
            std::string val = parser.get_option("--interval");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--interval requires a value in seconds\n";
                return 1;
            }
            try {
                interval = std::stoi(val);
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --interval: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--refresh-rate")) {
            std::string val = parser.get_option("--refresh-rate");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--refresh-rate requires a value in milliseconds\n";
                return 1;
            }
            try {
                refresh_ms = std::chrono::milliseconds(std::stoi(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --refresh-rate: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--cpu-poll")) {
            std::string val = parser.get_option("--cpu-poll");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--cpu-poll requires a value in seconds\n";
                return 1;
            }
            try {
                cpu_poll_sec = static_cast<unsigned int>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --cpu-poll: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--mem-poll")) {
            std::string val = parser.get_option("--mem-poll");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--mem-poll requires a value in seconds\n";
                return 1;
            }
            try {
                mem_poll_sec = static_cast<unsigned int>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --mem-poll: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--thread-poll")) {
            std::string val = parser.get_option("--thread-poll");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--thread-poll requires a value in seconds\n";
                return 1;
            }
            try {
                thread_poll_sec = static_cast<unsigned int>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --thread-poll: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--threads")) {
            std::string val = parser.get_option("--threads");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--threads requires a numeric value\n";
                return 1;
            }
            try {
                concurrency = static_cast<size_t>(std::stoul(val));
                if (concurrency == 0)
                    concurrency = 1;
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --threads: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--single-thread")) {
            concurrency = 1;
        }
        if (parser.has_flag("--concurrency")) {
            std::string val = parser.get_option("--concurrency");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--concurrency requires a numeric value\n";
                return 1;
            }
            try {
                concurrency = static_cast<size_t>(std::stoul(val));
                if (concurrency == 0)
                    concurrency = 1;
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --concurrency: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--max-threads")) {
            std::string val = parser.get_option("--max-threads");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--max-threads requires a numeric value\n";
                return 1;
            }
            try {
                max_threads = static_cast<size_t>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --max-threads: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--cpu-percent")) {
            std::string val = parser.get_option("--cpu-percent");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--cpu-percent requires a value\n";
                return 1;
            }
            if (!val.empty() && val.back() == '%')
                val.pop_back();
            try {
                cpu_percent_limit = std::stoi(val);
                if (cpu_percent_limit < 1 || cpu_percent_limit > 100)
                    throw 1;
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --cpu-percent: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--cpu-cores")) {
            std::string val = parser.get_option("--cpu-cores");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--cpu-cores requires a value\n";
                return 1;
            }
            try {
                cpu_core_mask = std::stoull(val, nullptr, 0);
                if (cpu_core_mask == 0)
                    throw 1;
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --cpu-cores: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--mem-limit")) {
            std::string val = parser.get_option("--mem-limit");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--mem-limit requires a numeric value\n";
                return 1;
            }
            try {
                mem_limit = static_cast<size_t>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --mem-limit: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--download-limit")) {
            std::string val = parser.get_option("--download-limit");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--download-limit requires a numeric value\n";
                return 1;
            }
            try {
                down_limit = static_cast<size_t>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --download-limit: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--upload-limit")) {
            std::string val = parser.get_option("--upload-limit");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--upload-limit requires a numeric value\n";
                return 1;
            }
            try {
                up_limit = static_cast<size_t>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --upload-limit: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--disk-limit")) {
            std::string val = parser.get_option("--disk-limit");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--disk-limit requires a numeric value\n";
                return 1;
            }
            try {
                disk_limit = static_cast<size_t>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --disk-limit: " << val << "\n";
                return 1;
            }
        }
        if (parser.has_flag("--max-depth")) {
            std::string val = parser.get_option("--max-depth");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--max-depth requires a numeric value\n";
                return 1;
            }
            try {
                max_depth = static_cast<size_t>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --max-depth: " << val << "\n";
                return 1;
            }
        }
        cpu_tracker = !parser.has_flag("--no-cpu-tracker");
        mem_tracker = !parser.has_flag("--no-mem-tracker");
        thread_tracker = !parser.has_flag("--no-thread-tracker");
        net_tracker = parser.has_flag("--net-tracker");
        debugMemory = parser.has_flag("--debug-memory");
        dumpState = parser.has_flag("--dump-state");
        if (parser.has_flag("--dump-large")) {
            std::string val = parser.get_option("--dump-large");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--dump-large requires a numeric value\n";
                return 1;
            }
            try {
                dumpThreshold = static_cast<size_t>(std::stoul(val));
            } catch (...) {
                if (!silent)
                    std::cerr << "Invalid value for --dump-large: " << val << "\n";
                return 1;
            }
        }
        if (max_threads > 0 && concurrency > max_threads)
            concurrency = max_threads;
        fs::path log_dir;
        if (parser.has_flag("--log-dir") || cfg_opts.count("--log-dir")) {
            std::string val = parser.get_option("--log-dir");
            if (val.empty())
                val = cfg_opt("--log-dir");
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--log-dir requires a path\n";
                return 1;
            }
            log_dir = val;
            try {
                if (fs::exists(log_dir) && !fs::is_directory(log_dir)) {
                    if (!silent)
                        std::cerr << "--log-dir path exists and is not a directory\n";
                    return 1;
                }
                fs::create_directories(log_dir);
            } catch (const std::exception& e) {
                if (!silent)
                    std::cerr << "Failed to create log directory: " << e.what() << "\n";
                return 1;
            }
        }
        std::string log_file;
        if (parser.has_flag("--log-file") || cfg_opts.count("--log-file")) {
            std::string val = parser.get_option("--log-file");
            if (val.empty())
                val = cfg_opt("--log-file");
            if (val.empty()) {
                std::string ts = timestamp();
                for (char& ch : ts) {
                    if (ch == ' ' || ch == ':')
                        ch = '_';
                    else if (ch == '/')
                        ch = '-';
                }
                log_file = "autogitpull-logs-" + ts + ".log";
            } else {
                log_file = val;
            }
        }
        fs::path root = parser.positional().front();
        if (!fs::exists(root) || !fs::is_directory(root)) {
            if (!silent)
                std::cerr << "Root path does not exist or is not a directory.\n";
            return 1;
        }
        std::vector<fs::path> ignore_dirs;
        for (const auto& val : parser.get_all_options("--ignore")) {
            if (val.empty()) {
                if (!silent)
                    std::cerr << "--ignore requires a path\n";
                return 1;
            }
            ignore_dirs.push_back(val);
        }
        if (cpu_core_mask != 0)
            procutil::set_cpu_affinity(cpu_core_mask);

        procutil::set_cpu_poll_interval(cpu_poll_sec);
        procutil::set_memory_poll_interval(mem_poll_sec);
        procutil::set_thread_poll_interval(thread_poll_sec);

        if (net_tracker)
            procutil::init_network_usage();

        if (!log_file.empty()) {
            init_logger(log_file, log_level);
            if (logger_initialized())
                log_info("Program started");
            else if (!silent)
                std::cerr << "Failed to open log file: " << log_file << "\n";
        }

        // Grab subdirectories at startup, skipping ignored paths
        std::vector<fs::path> all_repos =
            build_repo_list(root, recursive_scan, ignore_dirs, max_depth);
        std::map<fs::path, RepoInfo> repo_infos;
        for (const auto& p : all_repos) {
            repo_infos[p] = RepoInfo{p, RS_PENDING, "Pending...", "", "", "", 0, false};
        }

        std::set<fs::path> skip_repos;
        std::mutex mtx;
        std::atomic<bool> scanning(false);
        std::atomic<bool> running(true);
        std::string current_action = "Idle";
        std::mutex action_mtx;

        g_running_ptr = &running;
        std::signal(SIGINT, handle_signal);
#ifndef _WIN32
        std::signal(SIGTERM, handle_signal);
#endif

        ThreadGuard scan_thread;
        std::chrono::milliseconds countdown_ms(0); // Run immediately on start
        std::chrono::milliseconds cli_countdown_ms(0);

        std::unique_ptr<AltScreenGuard> guard;
        if (!cli && !silent)
            guard = std::make_unique<AltScreenGuard>();

        while (running) {
            if (!scanning && scan_thread.t.joinable()) {
                scan_thread.t.join();
                git_libgit2_shutdown();
                git_libgit2_init();
            }

            if (running && countdown_ms <= std::chrono::milliseconds(0) && !scanning) {
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    for (auto& [p, info] : repo_infos) {
                        if (info.status == RS_PULLING || info.status == RS_CHECKING) {
                            if (!silent)
                                std::cerr << "Manually clearing stale busy state for " << p << "\n";
                            info.status = RS_PENDING;
                            info.message = "Pending...";
                        }
                    }
                }
                scanning = true;
                scan_thread.t = std::thread(
                    scan_repos, std::cref(all_repos), std::ref(repo_infos), std::ref(skip_repos),
                    std::ref(mtx), std::ref(scanning), std::ref(running), std::ref(current_action),
                    std::ref(action_mtx), include_private, std::cref(log_dir), check_only,
                    hash_check, concurrency, cpu_percent_limit, mem_limit, down_limit, up_limit,
                    disk_limit, silent, force_pull);
                countdown_ms = std::chrono::seconds(interval);
            }
            {
                std::lock_guard<std::mutex> lk(mtx);
                int sec_left =
                    (int)std::chrono::duration_cast<std::chrono::seconds>(countdown_ms).count();
                if (sec_left < 0)
                    sec_left = 0;
                std::string act;
                {
                    std::lock_guard<std::mutex> a_lk(action_mtx);
                    act = current_action;
                }
                if (!silent && !cli) {
                    draw_tui(all_repos, repo_infos, interval, sec_left, scanning, act, show_skipped,
                             show_version, cpu_tracker, mem_tracker, thread_tracker, net_tracker,
                             cpu_core_mask != 0);
                } else if (!silent && cli && cli_countdown_ms <= std::chrono::milliseconds(0)) {
                    draw_cli(all_repos, repo_infos, sec_left, scanning, act, show_skipped);
                    cli_countdown_ms = std::chrono::milliseconds(1000);
                }
            }
            std::this_thread::sleep_for(refresh_ms);
            countdown_ms -= refresh_ms;
            cli_countdown_ms -= refresh_ms;
        }

        running = false;
        if (scan_thread.t.joinable()) {
            scan_thread.t.join();
            git_libgit2_shutdown();
            git_libgit2_init();
        }
        if (logger_initialized())
            log_info("Program exiting");
        shutdown_logger();
    } catch (...) {
        throw;
    }
    return 0;
}
#endif // AUTOGITPULL_NO_MAIN
