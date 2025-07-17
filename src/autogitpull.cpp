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
#include <cstring>
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
#include "parse_utils.hpp"
#include "options.hpp"

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

struct OptionInfo {
    const char* long_flag;
    const char* short_flag;
    const char* arg;
    const char* desc;
    const char* category;
};

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
    static const std::vector<OptionInfo> opts = {
        {"--include-private", "-p", "", "Include private repositories", "General"},
        {"--show-skipped", "-k", "", "Show skipped repositories", "General"},
        {"--show-version", "-v", "", "Display program version in TUI", "General"},
        {"--version", "-V", "", "Print program version and exit", "General"},
        {"--interval", "-i", "<sec>", "Delay between scans", "General"},
        {"--refresh-rate", "-r", "<ms>", "TUI refresh rate", "General"},
        {"--recursive", "", "", "Scan subdirectories recursively", "General"},
        {"--max-depth", "-D", "<n>", "Limit recursive scan depth", "General"},
        {"--ignore", "", "<dir>", "Directory to ignore (repeatable)", "General"},
        {"--config-yaml", "-y", "<file>", "Load options from YAML file", "General"},
        {"--config-json", "-j", "<file>", "Load options from JSON file", "General"},
        {"--cli", "-c", "", "Use console output", "General"},
        {"--silent", "-s", "", "Disable console output", "General"},
        {"--check-only", "", "", "Only check for updates", "Actions"},
        {"--no-hash-check", "", "", "Always pull without hash check", "Actions"},
        {"--force-pull", "", "", "Discard local changes when pulling", "Actions"},
        {"--discard-dirty", "", "", "Alias for --force-pull", "Actions"},
        {"--log-dir", "-d", "<path>", "Directory for pull logs", "Logging"},
        {"--log-file", "-l", "<path>", "File for general logs", "Logging"},
        {"--log-level", "", "<level>", "Set log verbosity", "Logging"},
        {"--verbose", "", "", "Shorthand for --log-level DEBUG", "Logging"},
        {"--debug-memory", "", "", "Log memory usage each scan", "Logging"},
        {"--dump-state", "", "", "Dump container state when large", "Logging"},
        {"--dump-large", "", "<n>", "Dump threshold for --dump-state", "Logging"},
        {"--concurrency", "", "<n>", "Number of worker threads", "Concurrency"},
        {"--threads", "", "<n>", "Alias for --concurrency", "Concurrency"},
        {"--single-thread", "", "", "Run using a single worker thread", "Concurrency"},
        {"--max-threads", "", "<n>", "Cap the scanning worker threads", "Concurrency"},
        {"--cpu-poll", "", "<s>", "CPU usage polling interval", "Tracking"},
        {"--mem-poll", "", "<s>", "Memory usage polling interval", "Tracking"},
        {"--thread-poll", "", "<s>", "Thread count polling interval", "Tracking"},
        {"--no-cpu-tracker", "", "", "Disable CPU usage tracker", "Tracking"},
        {"--no-mem-tracker", "", "", "Disable memory usage tracker", "Tracking"},
        {"--no-thread-tracker", "", "", "Disable thread tracker", "Tracking"},
        {"--net-tracker", "", "", "Track network usage", "Tracking"},
        {"--cpu-percent", "", "<n>", "Approximate CPU usage limit", "Resource limits"},
        {"--cpu-cores", "", "<mask>", "Set CPU affinity mask", "Resource limits"},
        {"--mem-limit", "", "<MB>", "Abort if memory exceeds this amount", "Resource limits"},
        {"--download-limit", "", "<KB/s>", "Limit total download rate", "Resource limits"},
        {"--upload-limit", "", "<KB/s>", "Limit total upload rate", "Resource limits"},
        {"--disk-limit", "", "<KB/s>", "Limit disk throughput", "Resource limits"},
        {"--help", "-h", "", "Show this message", "General"}};

    std::map<std::string, std::vector<const OptionInfo*>> groups;
    size_t width = 0;
    for (const auto& o : opts) {
        groups[o.category].push_back(&o);
        std::string flag = "  ";
        if (std::strlen(o.short_flag))
            flag += std::string(o.short_flag) + ", ";
        else
            flag += "    ";
        flag += o.long_flag;
        if (std::strlen(o.arg))
            flag += " " + std::string(o.arg);
        width = std::max(width, flag.size());
    }

    std::cout << "Usage: " << prog << " <root-folder> [options]\n\n";
    const std::vector<std::string> order{"General",         "Logging",  "Concurrency",
                                         "Resource limits", "Tracking", "Actions"};
    for (const auto& cat : order) {
        if (!groups.count(cat))
            continue;
        std::cout << cat << ":\n";
        for (const auto* o : groups[cat]) {
            std::string flag = "  ";
            if (std::strlen(o->short_flag))
                flag += std::string(o->short_flag) + ", ";
            else
                flag += "    ";
            flag += o->long_flag;
            if (std::strlen(o->arg))
                flag += " " + std::string(o->arg);
            std::cout << std::left << std::setw(static_cast<int>(width) + 2) << flag << o->desc
                      << "\n";
        }
        std::cout << "\n";
    }
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
    static size_t last_mem = 0;
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
    if (debugMemory || dumpState) {
        size_t mem_after = procutil::get_memory_usage_mb();
        size_t virt_after = procutil::get_virtual_memory_kb();
        log_debug("Memory before=" + std::to_string(mem_before) + "MB after=" +
                  std::to_string(mem_after) + "MB delta=" + std::to_string(mem_after - mem_before) +
                  "MB vmem_before=" + std::to_string(virt_before / 1024) +
                  "MB vmem_after=" + std::to_string(virt_after / 1024) +
                  "MB vmem_delta=" + std::to_string((virt_after - virt_before) / 1024) + "MB");
        debug_utils::log_memory_delta_mb(mem_after, last_mem);
        debug_utils::log_container_size("repo_infos", repo_infos);
        debug_utils::log_container_size("skip_repos", skip_repos);
        if (dumpState && repo_infos.size() > dumpThreshold)
            debug_utils::dump_repo_infos(repo_infos, dumpThreshold);
        if (dumpState && skip_repos.size() > dumpThreshold)
            debug_utils::dump_container("skip_repos", skip_repos, dumpThreshold);
    }
    scanning_flag = false;
    {
        std::lock_guard<std::mutex> lk(action_mtx);
        action = "Idle";
    }
    if (logger_initialized())
        log_debug("Scan complete");
}

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

    Options opts;
    opts.cli = parser.has_flag("--cli") || cfg_flag("--cli");
    opts.silent = parser.has_flag("--silent") || cfg_flag("--silent");
    opts.recursive_scan = parser.has_flag("--recursive") || cfg_flag("--recursive");
    opts.show_help = parser.has_flag("--help");
    opts.print_version = parser.has_flag("--version");
    if (parser.positional().size() != 1 && !opts.show_help && !opts.print_version)
        throw std::runtime_error("Root path required");
    if (!parser.unknown_flags().empty()) {
        throw std::runtime_error("Unknown option: " + parser.unknown_flags().front());
    }
    opts.include_private = parser.has_flag("--include-private") || cfg_flag("--include-private");
    opts.show_skipped = parser.has_flag("--show-skipped") || cfg_flag("--show-skipped");
    opts.show_version = parser.has_flag("--show-version") || cfg_flag("--show-version");
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
            opts.log_level = LogLevel::ERROR;
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

int run_event_loop(const Options& opts) {
    debugMemory = opts.debug_memory;
    dumpState = opts.dump_state;
    dumpThreshold = opts.dump_threshold;
    if (opts.root.empty())
        return 0;
    if (!fs::exists(opts.root) || !fs::is_directory(opts.root))
        throw std::runtime_error("Root path does not exist or is not a directory.");
    if (opts.cpu_core_mask != 0)
        procutil::set_cpu_affinity(opts.cpu_core_mask);
    procutil::set_cpu_poll_interval(opts.cpu_poll_sec);
    procutil::set_memory_poll_interval(opts.mem_poll_sec);
    procutil::set_thread_poll_interval(opts.thread_poll_sec);
    if (opts.net_tracker)
        procutil::init_network_usage();
    if (!opts.log_file.empty()) {
        init_logger(opts.log_file, opts.log_level);
        if (logger_initialized())
            log_info("Program started");
    }
    fs::create_directories(opts.log_dir);
    std::vector<fs::path> all_repos =
        build_repo_list(opts.root, opts.recursive_scan, opts.ignore_dirs, opts.max_depth);
    std::map<fs::path, RepoInfo> repo_infos;
    for (const auto& p : all_repos)
        repo_infos[p] = RepoInfo{p, RS_PENDING, "Pending...", "", "", "", 0, false};
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
    std::chrono::milliseconds countdown_ms(0);
    std::chrono::milliseconds cli_countdown_ms(0);
    std::unique_ptr<AltScreenGuard> guard;
    if (!opts.cli && !opts.silent)
        guard = std::make_unique<AltScreenGuard>();
    size_t concurrency = opts.concurrency;
    if (opts.max_threads > 0 && concurrency > opts.max_threads)
        concurrency = opts.max_threads;
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
                        if (!opts.silent)
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
                std::ref(action_mtx), opts.include_private, std::cref(opts.log_dir),
                opts.check_only, opts.hash_check, concurrency, opts.cpu_percent_limit,
                opts.mem_limit, opts.download_limit, opts.upload_limit, opts.disk_limit,
                opts.silent, opts.force_pull);
            countdown_ms = std::chrono::seconds(opts.interval);
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
            if (!opts.silent && !opts.cli) {
                draw_tui(all_repos, repo_infos, opts.interval, sec_left, scanning, act,
                         opts.show_skipped, opts.show_version, opts.cpu_tracker, opts.mem_tracker,
                         opts.thread_tracker, opts.net_tracker, opts.cpu_core_mask != 0);
            } else if (!opts.silent && opts.cli &&
                       cli_countdown_ms <= std::chrono::milliseconds(0)) {
                draw_cli(all_repos, repo_infos, sec_left, scanning, act, opts.show_skipped);
                cli_countdown_ms = std::chrono::milliseconds(1000);
            }
        }
        std::this_thread::sleep_for(opts.refresh_ms);
        countdown_ms -= opts.refresh_ms;
        cli_countdown_ms -= opts.refresh_ms;
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
    return 0;
}

#ifndef AUTOGITPULL_NO_MAIN
int main(int argc, char* argv[]) {
    git::GitInitGuard git_guard;
    try {
        Options opts = parse_options(argc, argv);
        if (opts.show_help) {
            print_help(argv[0]);
            return 0;
        }
        if (opts.print_version) {
            std::cout << AUTOGITPULL_VERSION << "\n";
            return 0;
        }
        return run_event_loop(opts);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
#endif // AUTOGITPULL_NO_MAIN
