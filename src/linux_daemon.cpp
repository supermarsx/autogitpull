#include "linux_daemon.hpp"
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

static fs::path unit_dir() {
    const char* env = std::getenv("AUTOGITPULL_UNIT_DIR");
    if (env && *env)
        return env;
    return "/etc/systemd/system";
}

static std::string unit_path(const std::string& name) {
    return (unit_dir() / (name + ".service")).string();
}

bool create_service_unit(const std::string& name, const std::string& exec_path,
                         const std::string& config_file, const std::string& user, bool persist) {
    std::ofstream out(unit_path(name));
    if (!out.is_open())
        return false;
    out << "[Unit]\nDescription=autogitpull daemon\nAfter=network.target\n\n";
    out << "[Service]\nType=simple\nUser=" << user << "\nExecStart=" << exec_path;
    if (!config_file.empty())
        out << " --daemon-config " << config_file;
    if (persist)
        out << " --persist";
    out << "\nRestart=on-failure\n\n";
    out << "[Install]\nWantedBy=multi-user.target\n";
    out.close();
    std::system("systemctl daemon-reload > /dev/null 2>&1");
    return true;
}

bool remove_service_unit(const std::string& name) {
    std::string path = unit_path(name);
    std::remove(path.c_str());
    std::system("systemctl daemon-reload > /dev/null 2>&1");
    return true;
}

bool service_unit_exists(const std::string& name) { return fs::exists(unit_path(name)); }

bool start_service_unit(const std::string& name) {
    std::string cmd = "systemctl start " + name + " > /dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}

bool stop_service_unit(const std::string& name, bool force) {
    std::string cmd = "systemctl stop " + name + " > /dev/null 2>&1";
    bool ok = std::system(cmd.c_str()) == 0;
    if (!ok && force) {
        std::string killcmd = "systemctl kill -s SIGKILL " + name + " > /dev/null 2>&1";
        std::system(killcmd.c_str());
        ok = std::system(cmd.c_str()) == 0;
    }
    return ok;
}

bool restart_service_unit(const std::string& name, bool force) {
    std::string cmd = "systemctl restart " + name + " > /dev/null 2>&1";
    bool ok = std::system(cmd.c_str()) == 0;
    if (!ok && force) {
        stop_service_unit(name, true);
        ok = std::system(cmd.c_str()) == 0;
    }
    return ok;
}

bool service_unit_status(const std::string& name, ServiceStatus& out) {
    std::string status_cmd = "systemctl status " + name + " > /dev/null 2>&1";
    out.exists = std::system(status_cmd.c_str()) == 0;
    std::string active_cmd = "systemctl is-active --quiet " + name + " > /dev/null 2>&1";
    out.running = std::system(active_cmd.c_str()) == 0;
    return out.exists;
}

std::vector<std::pair<std::string, ServiceStatus>> list_installed_services() {
    std::vector<std::pair<std::string, ServiceStatus>> result;
    fs::path dir = unit_dir();
    if (!fs::exists(dir))
        return result;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".service")
            continue;
        std::ifstream in(entry.path());
        if (!in.is_open())
            continue;
        std::string line;
        bool ours = false;
        while (std::getline(in, line)) {
            if (line.rfind("ExecStart=", 0) == 0 && line.find("autogitpull") != std::string::npos) {
                ours = true;
                break;
            }
        }
        if (ours) {
            std::string name = entry.path().stem().string();
            ServiceStatus st{};
            service_unit_status(name, st);
            result.emplace_back(name, st);
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
#endif

} // namespace procutil
