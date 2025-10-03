#include "lock_utils.hpp"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/socket.h>
#include <sys/un.h>
#endif
#ifdef __APPLE__
#include <libproc.h>
#include <sys/sysctl.h>
#endif
#include <algorithm>
#include <cstring>
#include <fstream>
#include <set>
#include <system_error>
#include "system_utils.hpp"

namespace procutil {

bool acquire_lock_file(const std::filesystem::path& path) {
    UniqueFd fd(open(path.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644));
    if (!fd)
        return false;
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%ld\n", static_cast<long>(getpid()));
    ssize_t w = write(fd.get(), buf, static_cast<size_t>(len));
    (void)w;
    return true;
}

void release_lock_file(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    (void)ec;
}

bool read_lock_pid(const std::filesystem::path& path, unsigned long& pid) {
    std::ifstream f(path);
    if (!f.is_open())
        return false;
    f >> pid;
    return true;
}

bool process_running(unsigned long pid) {
    if (kill(static_cast<pid_t>(pid), 0) == 0)
        return true;
    return errno != ESRCH;
}

bool terminate_process(unsigned long pid) { return kill(static_cast<pid_t>(pid), SIGTERM) == 0; }

LockFileGuard::LockFileGuard(const std::filesystem::path& p) : path(p) {
    locked = acquire_lock_file(path);
}

LockFileGuard::~LockFileGuard() {
    if (locked)
        release_lock_file(path);
}

std::vector<std::pair<std::string, unsigned long>> find_running_instances() {
    namespace fs = std::filesystem;
    std::vector<std::pair<std::string, unsigned long>> out;
    auto add_inst = [&](const std::string& n, unsigned long p) { out.emplace_back(n, p); };

    fs::path tmp = fs::temp_directory_path();
    for (const auto& entry :
         fs::directory_iterator(tmp, fs::directory_options::skip_permission_denied)) {
        std::error_code ec;
        const auto status = entry.status(ec);
        if (!ec && fs::is_directory(status)) {
            fs::path lock = entry.path() / ".autogitpull.lock";
            unsigned long pid = 0;
            std::error_code ec2;
            if (fs::exists(lock, ec2) && !ec2 && read_lock_pid(lock, pid) && process_running(pid))
                add_inst(entry.path().filename().string(), pid);
        }
#ifdef __linux__
        if (entry.path().extension() == ".sock") {
            UniqueFd fd(socket(AF_UNIX, SOCK_STREAM, 0));
            if (fd) {
                sockaddr_un addr{};
                addr.sun_family = AF_UNIX;
                std::strncpy(addr.sun_path, entry.path().c_str(), sizeof(addr.sun_path) - 1);
                if (connect(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
                    ucred cred{};
                    socklen_t len = sizeof(cred);
                    if (getsockopt(fd.get(), SOL_SOCKET, SO_PEERCRED, &cred, &len) == 0)
                        add_inst(entry.path().stem().string(), cred.pid);
                }
            }
        }
#endif
    }

#ifdef __linux__
    fs::path proc_dir("/proc");
    for (const auto& entry : fs::directory_iterator(proc_dir)) {
        if (!entry.is_directory())
            continue;
        std::string pidstr = entry.path().filename().string();
        if (!std::all_of(pidstr.begin(), pidstr.end(), ::isdigit))
            continue;
        unsigned long pid = 0;
        try {
            pid = std::stoul(pidstr);
        } catch (...) {
            continue;
        }
        std::ifstream f(entry.path() / "cmdline");
        if (!f.is_open())
            continue;
        std::string arg0;
        std::getline(f, arg0, '\0');
        if (fs::path(arg0).filename() == "autogitpull")
            add_inst("autogitpull", pid);
    }
#elif defined(__APPLE__)
    auto is_autogitpull_process = [](pid_t pid) -> bool {
        char namebuf[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_name(pid, namebuf, sizeof(namebuf)) > 0) {
            if (std::string(namebuf) == "autogitpull")
                return true;
        }

        int mib[3] = {CTL_KERN, KERN_PROCARGS2, pid};
        size_t args_size = 0;
        if (sysctl(mib, 3, nullptr, &args_size, nullptr, 0) != 0 || args_size == 0)
            return false;
        std::vector<char> args(args_size);
        if (sysctl(mib, 3, args.data(), &args_size, nullptr, 0) != 0 || args_size == 0)
            return false;

        const char* data = args.data();
        if (args_size < sizeof(int))
            return false;
        int argc = *reinterpret_cast<const int*>(data);
        if (argc <= 0)
            return false;
        const char* ptr = data + sizeof(int);
        const char* end = data + args_size;

        while (ptr < end && *ptr != '\0')
            ++ptr;
        if (ptr >= end)
            return false;
        ++ptr;

        if (ptr >= end || *ptr == '\0')
            return false;
        std::string argv0(ptr);
        if (argv0 == "autogitpull")
            return true;
        std::string base = std::filesystem::path(argv0).filename().string();
        return base == "autogitpull";
    };

    int count = proc_listallpids(nullptr, 0);
    if (count > 0) {
        std::vector<pid_t> pids(static_cast<std::size_t>(count));
        count = proc_listallpids(pids.data(), static_cast<int>(pids.size() * sizeof(pid_t)));
        count /= static_cast<int>(sizeof(pid_t));
        for (int i = 0; i < count; ++i) {
            if (is_autogitpull_process(pids[i]))
                add_inst("autogitpull", static_cast<unsigned long>(pids[i]));
        }
    }
#endif

    return out;
}

} // namespace procutil
