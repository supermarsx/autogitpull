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
        {"--include-private", "-p", "", "Include private repositories", "Basics"},
        {"--root", "-o", "<path>", "Root folder of repositories", "Basics"},
        {"--interval", "-i", "<sec>", "Delay between scans", "Basics"},
        {"--refresh-rate", "-r", "<ms>", "TUI refresh rate", "Basics"},
        {"--recursive", "-e", "", "Scan subdirectories recursively", "Basics"},
        {"--max-depth", "-D", "<n>", "Limit recursive scan depth", "Basics"},
        {"--ignore", "-I", "<dir>", "Directory to ignore (repeatable)", "Basics"},
        {"--single-run", "-u", "", "Run a single scan cycle and exit", "Basics"},
        {"--single-repo", "-S", "", "Only monitor the specified root repo", "Basics"},
        {"--rescan-new", "-w", "<min>", "Rescan for new repos every N minutes (default 5)",
         "Basics"},
        {"--wait-empty", "-W", "[n]", "Keep retrying when no repos are found (optional limit)",
         "Basics"},
        {"--dont-skip-timeouts", "", "", "Retry repositories that timeout", "Basics"},
        {"--keep-first-valid", "", "", "Keep valid repos from first scan", "Basics"},
        {"--updated-since", "", "<N[m|h|d|w|M]>", "Only sync repos updated recently", "Basics"},
        {"--cli", "-c", "", "Use console output", "Process"},
        {"--silent", "-s", "", "Disable console output", "Process"},
        {"--attach", "-A", "<name>", "Attach to daemon and show status", "Process"},
        {"--background", "-b", "<name>", "Run in background with attach name", "Process"},
        {"--reattach", "-B", "<name>", "Reattach to background process", "Process"},
        {"--persist", "-P", "[name]", "Keep running after exit (optional name)", "Process"},
        {"--respawn-limit", "", "<n[,min]>", "Respawn limit within minutes", "Process"},
        {"--max-runtime", "", "<sec>", "Exit after given runtime", "Process"},
        {"--pull-timeout", "-O", "<sec>", "Network operation timeout", "Process"},
        {"--print-skipped", "", "", "Print skipped repositories once", "Process"},
        {"--keep-first", "", "", "Keep repos validated on first scan", "Process"},
        {"--auto-config", "", "", "Auto detect YAML or JSON config", "Config"},
        {"--auto-reload-config", "", "", "Reload config when the file changes", "Config"},
        {"--rerun-last", "", "", "Reuse args from .autogitpull.config", "Config"},
        {"--save-args", "", "", "Save args to config file", "Config"},
        {"--enable-history", "", "[=file]", "Enable command history", "Config"},
        {"--enable-hotkeys", "", "", "Enable TUI hotkeys", "Config"},
        {"--config-yaml", "-y", "<file>", "Load options from YAML file", "Config"},
        {"--config-json", "-j", "<file>", "Load options from JSON file", "Config"},
        {"--show-skipped", "-k", "", "Show skipped repositories", "Display"},
        {"--show-version", "-v", "", "Display program version in TUI", "Display"},
        {"--version", "-V", "", "Print program version and exit", "Display"},
        {"--show-runtime", "", "", "Display elapsed runtime", "Display"},
        {"--show-repo-count", "-Z", "", "Display number of repositories", "Display"},
        {"--show-commit-date", "-T", "", "Display last commit time", "Display"},
        {"--show-commit-author", "-U", "", "Display last commit author", "Display"},
        {"--show-pull-author", "", "", "Show author when pull succeeds", "Display"},
        {"--session-dates-only", "", "", "Only show dates for repos pulled this session",
         "Display"},
        {"--hide-date-time", "", "", "Hide date/time line in TUI", "Display"},
        {"--hide-header", "-H", "", "Hide status header", "Display"},
        {"--row-order", "", "<mode>", "Row ordering (alpha/reverse)", "Display"},
        {"--color", "", "<ansi>", "Override status color", "Display"},
        {"--no-colors", "-C", "", "Disable ANSI colors", "Display"},
        {"--check-only", "-x", "", "Only check for updates", "Actions"},
        {"--no-hash-check", "-N", "", "Always pull without hash check", "Actions"},
        {"--force-pull", "-f", "", "Discard local changes when pulling", "Actions"},
        {"--discard-dirty", "", "", "Alias for --force-pull", "Actions"},
        {"--install-daemon", "", "", "Install background daemon", "Actions"},
        {"--uninstall-daemon", "", "", "Uninstall background daemon", "Actions"},
        {"--daemon-config", "", "<file>", "Config file for daemon install", "Actions"},
        {"--install-service", "", "", "Install system service", "Actions"},
        {"--uninstall-service", "", "", "Uninstall system service", "Actions"},
        {"--start-service", "", "", "Start installed service", "Actions"},
        {"--stop-service", "", "", "Stop installed service", "Actions"},
        {"--force-stop-service", "", "", "Force stop service", "Actions"},
        {"--restart-service", "", "", "Restart service", "Actions"},
        {"--force-restart-service", "", "", "Force restart service", "Actions"},
        {"--service-config", "", "<file>", "Config file for service install", "Actions"},
        {"--service-name", "", "<name>", "Service name for install", "Actions"},
        {"--daemon-name", "", "<name>", "Daemon unit name for install", "Actions"},
        {"--start-daemon", "", "", "Start daemon service", "Actions"},
        {"--stop-daemon", "", "", "Stop daemon service", "Actions"},
        {"--force-stop-daemon", "", "", "Force stop daemon", "Actions"},
        {"--restart-daemon", "", "", "Restart daemon service", "Actions"},
        {"--force-restart-daemon", "", "", "Force restart daemon", "Actions"},
        {"--service-status", "", "", "Check service existence and running state", "Actions"},
        {"--daemon-status", "", "", "Check daemon existence and running state", "Actions"},
        {"--show-service", "", "", "Show installed service name", "Actions"},
        {"--remove-lock", "-R", "", "Remove directory lock file and exit", "Actions"},
        {"--kill-all", "", "", "Terminate running instance and exit", "Actions"},
        {"--kill-on-sleep", "", "", "Exit if a system sleep is detected", "Actions"},
        {"--list-instances", "", "", "List running instance names and PIDs", "Actions"},
        {"--list-services", "", "", "List installed service units", "Actions"},
        {"--list-daemons", "", "", "Alias for --list-services", "Actions"},
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
        {"--mem-limit", "-Y", "<size>", "Abort if memory exceeds this amount", "Resource limits"},
        {"--download-limit", "", "<rate>", "Limit total download rate", "Resource limits"},
        {"--upload-limit", "", "<rate>", "Limit total upload rate", "Resource limits"},
        {"--total-traffic-limit", "", "<size>", "Abort if network traffic exceeds this amount",
         "Resource limits"},
        {"--show-commit-date", "-T", "", "Display last commit time", "Display"},
        {"--disk-limit", "", "<rate>", "Limit disk throughput", "Resource limits"},
        {"--show-commit-author", "-U", "", "Display last commit author", "Display"},
        {"--hide-date-time", "", "", "Hide date/time line in TUI", "Display"},
        {"--hide-header", "-H", "", "Hide status header", "Display"},
        {"--row-order", "", "<mode>", "Row ordering (alpha/reverse)", "Display"},
        {"--color", "", "<ansi>", "Override status color", "Display"},
        {"--no-colors", "-C", "", "Disable ANSI colors", "Display"},
        {"--vmem", "", "", "Show virtual memory usage", "Tracking"},
        {"--syslog", "", "", "Log to syslog", "Logging"},
        {"--syslog-facility", "", "<n>", "Syslog facility", "Logging"},
        {"--pull-timeout", "-O", "<sec>", "Network operation timeout", "Process"},
        {"--help", "-h", "", "Show this message", "Basics"}};

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
    const std::vector<std::string> order{"Basics",          "Display",  "Config",
                                         "Process",         "Logging",  "Concurrency",
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
