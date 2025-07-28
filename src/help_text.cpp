#include "help_text.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <cstring>
#include <string>

struct OptionInfo {
    const char* long_flag;
    const char* short_flag;
    const char* arg;
    const char* desc;
    const char* category;
};

void print_help(const char* prog) {
    static const std::vector<OptionInfo> opts = {
        {"--include-private", "-p", "", "Include private repositories", "General"},
        {"--show-skipped", "-k", "", "Show skipped repositories", "General"},
        {"--show-version", "-v", "", "Display program version in TUI", "General"},
        {"--version", "-V", "", "Print program version and exit", "General"},
        {"--interval", "-i", "<sec>", "Delay between scans", "General"},
        {"--refresh-rate", "-r", "<ms>", "TUI refresh rate", "General"},
        {"--recursive", "-e", "", "Scan subdirectories recursively", "General"},
        {"--max-depth", "-D", "<n>", "Limit recursive scan depth", "General"},
        {"--ignore", "-I", "<dir>", "Directory to ignore (repeatable)", "General"},
        {"--config-yaml", "-y", "<file>", "Load options from YAML file", "General"},
        {"--config-json", "-j", "<file>", "Load options from JSON file", "General"},
        {"--root", "-o", "<path>", "Root folder of repositories", "General"},
        {"--cli", "-c", "", "Use console output", "General"},
        {"--single-run", "-u", "", "Run a single scan cycle and exit", "General"},
        {"--single-repo", "-S", "", "Only monitor the specified root repo", "General"},
        {"--rescan-new", "-w", "<min>", "Rescan for new repos every N minutes (default 5)",
         "General"},
        {"--wait-empty", "-W", "", "Keep retrying when no repos are found", "General"},
        {"--dont-skip-timeouts", "", "", "Retry repositories that timeout", "General"},
        {"--silent", "-s", "", "Disable console output", "General"},
        {"--attach", "-A", "<name>", "Attach to daemon and show status", "General"},
        {"--background", "-b", "<name>", "Run in background with attach name", "General"},
        {"--reattach", "-B", "<name>", "Reattach to background process", "General"},
        {"--show-runtime", "", "", "Display elapsed runtime", "General"},
        {"--show-repo-count", "-Z", "", "Display number of repositories", "General"},
        {"--max-runtime", "", "<sec>", "Exit after given runtime", "General"},
        {"--persist", "-P", "", "Keep running after exit", "General"},
        {"--respawn-limit", "", "<n[,min]>", "Respawn limit within minutes", "General"},
        {"--updated-since", "", "<N[m|h|d|w|M]>", "Only sync repos updated recently", "General"},
        {"--auto-config", "", "", "Auto detect YAML or JSON config", "General"},
        {"--auto-reload-config", "", "", "Reload config when the file changes", "General"},
        {"--rerun-last", "", "", "Reuse args from .autogitpull.config", "General"},
        {"--save-args", "", "", "Save args to config file", "General"},
        {"--enable-history", "", "", "Enable command history", "General"},
        {"--session-dates-only", "", "", "Only show dates for repos pulled this session",
         "General"},
        {"--check-only", "-x", "", "Only check for updates", "Actions"},
        {"--no-hash-check", "-N", "", "Always pull without hash check", "Actions"},
        {"--force-pull", "-f", "", "Discard local changes when pulling", "Actions"},
        {"--discard-dirty", "", "", "Alias for --force-pull", "Actions"},
        {"--install-daemon", "", "", "Install background daemon", "Actions"},
        {"--uninstall-daemon", "", "", "Uninstall background daemon", "Actions"},
        {"--daemon-config", "", "<file>", "Config file for daemon install", "Actions"},
        {"--install-service", "", "", "Install system service", "Actions"},
        {"--uninstall-service", "", "", "Uninstall system service", "Actions"},
        {"--service-config", "", "<file>", "Config file for service install", "Actions"},
        {"--remove-lock", "-R", "", "Remove directory lock file and exit", "Actions"},
        {"--kill-all", "", "", "Terminate running instance and exit", "Actions"},
        {"--ignore-lock", "", "", "Don't create or check lock file", "Actions"},
        {"--log-dir", "-d", "<path>", "Directory for pull logs", "Logging"},
        {"--log-file", "-l", "<path>", "File for general logs", "Logging"},
        {"--log-level", "-L", "<level>", "Set log verbosity", "Logging"},
        {"--verbose", "-g", "", "Shorthand for --log-level DEBUG", "Logging"},
        {"--debug-memory", "-m", "", "Log memory usage each scan", "Logging"},
        {"--dump-state", "", "", "Dump container state when large", "Logging"},
        {"--dump-large", "", "<n>", "Dump threshold for --dump-state", "Logging"},
        {"--concurrency", "-n", "<n>", "Number of worker threads", "Concurrency"},
        {"--threads", "-t", "<n>", "Alias for --concurrency", "Concurrency"},
        {"--single-thread", "-q", "", "Run using a single worker thread", "Concurrency"},
        {"--max-threads", "-M", "<n>", "Cap the scanning worker threads", "Concurrency"},
        {"--cpu-poll", "", "<s>", "CPU usage polling interval", "Tracking"},
        {"--mem-poll", "", "<s>", "Memory usage polling interval", "Tracking"},
        {"--thread-poll", "", "<s>", "Thread count polling interval", "Tracking"},
        {"--no-cpu-tracker", "-X", "", "Disable CPU usage tracker", "Tracking"},
        {"--no-mem-tracker", "", "", "Disable memory usage tracker", "Tracking"},
        {"--no-thread-tracker", "", "", "Disable thread tracker", "Tracking"},
        {"--net-tracker", "", "", "Track network usage", "Tracking"},
        {"--cpu-percent", "-E", "<n>", "Approximate CPU usage limit", "Resource limits"},
        {"--cpu-cores", "", "<mask>", "Set CPU affinity mask", "Resource limits"},
        {"--mem-limit", "-Y", "<MB>", "Abort if memory exceeds this amount", "Resource limits"},
        {"--download-limit", "", "<KB/s>", "Limit total download rate", "Resource limits"},
        {"--upload-limit", "", "<KB/s>", "Limit total upload rate", "Resource limits"},
        {"--show-commit-date", "-T", "", "Display last commit time", "General"},
        {"--disk-limit", "", "<KB/s>", "Limit disk throughput", "Resource limits"},
        {"--show-commit-author", "-U", "", "Display last commit author", "General"},
        {"--hide-date-time", "", "", "Hide date/time line in TUI", "General"},
        {"--hide-header", "-H", "", "Hide status header", "General"},
        {"--row-order", "", "<mode>", "Row ordering (alpha/reverse)", "General"},
        {"--color", "", "<ansi>", "Override status color", "General"},
        {"--no-colors", "-C", "", "Disable ANSI colors", "General"},
        {"--vmem", "", "", "Show virtual memory usage", "Tracking"},
        {"--syslog", "", "", "Log to syslog", "Logging"},
        {"--syslog-facility", "", "<n>", "Syslog facility", "Logging"},
        {"--pull-timeout", "-O", "<sec>", "Network operation timeout", "General"},
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

    std::cout << "autogitpull - Automatic Git Puller & Monitor\n";
    std::cout << "Scans a directory of Git repositories and pulls updates.\n";
    std::cout << "Configuration can be read from YAML or JSON files.\n\n";
    std::cout << "Usage: " << prog << " <root-folder> [options]\n";
    std::cout << "       " << prog << " --root <path> [options]\n\n";
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
