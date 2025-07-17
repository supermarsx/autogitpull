#include "lockfile.hpp"
#include <fstream>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif

namespace {

#ifdef _WIN32
static bool process_running(int pid) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (!h)
        return false;
    DWORD code = 0;
    bool running = GetExitCodeProcess(h, &code) && code == STILL_ACTIVE;
    CloseHandle(h);
    return running;
}
static int current_pid() { return static_cast<int>(GetCurrentProcessId()); }
#else
static bool process_running(int pid) {
    if (pid <= 0)
        return false;
    return kill(pid, 0) == 0 || errno != ESRCH;
}
static int current_pid() { return static_cast<int>(getpid()); }
#endif

} // namespace

LockFile::LockFile(const std::filesystem::path& dir) : lock_dir_(dir / ".autogitpull-lock") {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (fs::exists(lock_dir_)) {
        std::ifstream f(lock_dir_ / "pid");
        int pid = 0;
        if (f)
            f >> pid;
        if (pid != 0 && process_running(pid)) {
            err_ = "Another instance is already running (PID " + std::to_string(pid) + ")";
            return;
        }
        fs::remove_all(lock_dir_, ec);
    }
    if (!fs::create_directory(lock_dir_, ec)) {
        err_ = "Failed to create lock directory";
        return;
    }
    std::ofstream(lock_dir_ / "pid") << current_pid();
    locked_ = true;
}

LockFile::~LockFile() {
    if (locked_) {
        std::error_code ec;
        std::filesystem::remove_all(lock_dir_, ec);
    }
}

bool LockFile::acquired() const { return locked_; }
const std::string& LockFile::error() const { return err_; }
