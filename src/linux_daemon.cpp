#include "linux_daemon.hpp"
#include <fstream>
#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

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

static std::string unit_path(const std::string& name) {
    return "/etc/systemd/system/" + name + ".service";
}

bool create_service_unit(const std::string& name, const std::string& exec_path,
                         const std::string& config_file, const std::string& user) {
    std::ofstream out(unit_path(name));
    if (!out.is_open())
        return false;
    out << "[Unit]\nDescription=autogitpull daemon\nAfter=network.target\n\n";
    out << "[Service]\nType=simple\nUser=" << user << "\nExecStart=" << exec_path;
    if (!config_file.empty())
        out << " --daemon-config " << config_file;
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
#else
bool daemonize() { return false; }

bool create_service_unit(const std::string&, const std::string&, const std::string&,
                         const std::string&) {
    return false;
}

bool remove_service_unit(const std::string&) { return false; }
#endif

} // namespace procutil
