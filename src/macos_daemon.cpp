#include "macos_daemon.hpp"
#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cstring>
#endif
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstdlib>

namespace fs = std::filesystem;

namespace procutil {

#ifndef _WIN32
bool daemonize() {
    pid_t pid = fork();
    if (pid < 0)
        return false;
    if (pid > 0)
        _exit(0);

    if (setsid() < 0)
        return false;

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if (pid < 0)
        return false;
    if (pid > 0)
        _exit(0);

    umask(0);
    if (chdir("/") != 0)
        return false;

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);
    return true;
}

static fs::path plist_dir() {
    const char* env = std::getenv("AUTOGITPULL_UNIT_DIR");
    if (env && *env)
        return env;
    const char* home = std::getenv("HOME");
    if (home && *home)
        return fs::path(home) / "Library/LaunchAgents";
    return "/Library/LaunchDaemons";
}

std::string plist_path(const std::string& name) {
    return (plist_dir() / (name + ".plist")).string();
}

std::string launchctl_command(const std::string& action, const std::string& name) {
    return "launchctl " + action + " " + plist_path(name) + " > /dev/null 2>&1";
}

bool create_service_unit(const std::string& name, const std::string& exec_path,
                         const std::string& config_file, const std::string& user, bool persist) {
    (void)user;
    std::ofstream out(plist_path(name));
    if (!out.is_open())
        return false;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
           "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    out << "<plist version=\"1.0\">\n<dict>\n";
    out << "    <key>Label</key><string>" << name << "</string>\n";
    out << "    <key>ProgramArguments</key>\n    <array>\n";
    out << "        <string>" << exec_path << "</string>\n";
    if (!config_file.empty()) {
        out << "        <string>--daemon-config</string>\n";
        out << "        <string>" << config_file << "</string>\n";
    }
    if (persist)
        out << "        <string>--persist</string>\n";
    out << "    </array>\n";
    out << "    <key>RunAtLoad</key><true/>\n";
    out << "</dict>\n</plist>\n";
    out.close();
    std::string cmd = launchctl_command("load", name);
    std::system(cmd.c_str());
    return true;
}

bool remove_service_unit(const std::string& name) {
    std::string cmd = launchctl_command("unload", name);
    std::system(cmd.c_str());
    std::remove(plist_path(name).c_str());
    return true;
}

bool service_unit_exists(const std::string& name) { return fs::exists(plist_path(name)); }

bool start_service_unit(const std::string& name) {
    std::string cmd = "launchctl start " + name + " > /dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}

bool stop_service_unit(const std::string& name, bool force) {
    (void)force;
    std::string cmd = "launchctl stop " + name + " > /dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}

bool restart_service_unit(const std::string& name, bool force) {
    bool ok = stop_service_unit(name, force);
    if (!ok && !force)
        return false;
    return start_service_unit(name);
}

bool service_unit_status(const std::string& name, ServiceStatus& out) {
    out.exists = service_unit_exists(name);
    out.running = false;
    if (!out.exists)
        return false;
    std::string cmd = "launchctl list " + name + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return false;
    char buf[256];
    if (fgets(buf, sizeof(buf), pipe)) {
        std::string line(buf);
        size_t tab = line.find('\t');
        std::string pid = line.substr(0, tab);
        out.running = pid != "0" && pid != "-";
    }
    pclose(pipe);
    return true;
}

std::vector<std::pair<std::string, ServiceStatus>> list_installed_services() {
    std::vector<std::pair<std::string, ServiceStatus>> result;
    fs::path dir = plist_dir();
    if (!fs::exists(dir))
        return result;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".plist")
            continue;
        std::ifstream in(entry.path());
        if (!in.is_open())
            continue;
        std::string line;
        bool ours = false;
        while (std::getline(in, line)) {
            if (line.find("autogitpull") != std::string::npos) {
                ours = true;
                break;
            }
        }
        if (ours) {
            std::string nm = entry.path().stem().string();
            ServiceStatus st{};
            service_unit_status(nm, st);
            result.emplace_back(nm, st);
        }
    }
    return result;
}

int create_status_socket(const std::string& name) {
    std::string path = "/tmp/" + name + ".sock";
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    unlink(path.c_str());
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || listen(fd, 5) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int connect_status_socket(const std::string& name) {
    std::string path = "/tmp/" + name + ".sock";
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

void remove_status_socket(const std::string& name, int fd) {
    if (fd >= 0)
        close(fd);
    std::string path = "/tmp/" + name + ".sock";
    unlink(path.c_str());
}

#else

bool daemonize() { return false; }

bool create_service_unit(const std::string&, const std::string&, const std::string&,
                         const std::string&, bool) {
    return false;
}

bool remove_service_unit(const std::string&) { return false; }
bool service_unit_exists(const std::string&) { return false; }
bool start_service_unit(const std::string&) { return false; }
bool stop_service_unit(const std::string&, bool) { return false; }
bool restart_service_unit(const std::string&, bool) { return false; }
bool service_unit_status(const std::string&, ServiceStatus&) { return false; }
std::vector<std::pair<std::string, ServiceStatus>> list_installed_services() { return {}; }
std::string plist_path(const std::string& name) { return name; }
std::string launchctl_command(const std::string& action, const std::string& name) {
    return action + " " + name;
}

#endif

} // namespace procutil
