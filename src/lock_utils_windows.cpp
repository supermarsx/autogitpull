#include "lock_utils.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <set>
#include <system_error>
#include "system_utils.hpp"

namespace procutil {

bool acquire_lock_file(const std::filesystem::path& path) {
    UniqueHandle h(CreateFileW(path.wstring().c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL, NULL));
    if (!h)
        return false;
    DWORD pid = GetCurrentProcessId();
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%lu\n", static_cast<unsigned long>(pid));
    DWORD written = 0;
    WriteFile(h.get(), buf, len, &written, NULL);
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
    UniqueHandle h(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid)));
    if (!h)
        return false;
    DWORD exit_code = 0;
    bool running = GetExitCodeProcess(h.get(), &exit_code) && exit_code == STILL_ACTIVE;
    return running;
}

bool terminate_process(unsigned long pid) {
    UniqueHandle h(OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid)));
    if (!h)
        return false;
    return TerminateProcess(h.get(), 0) != 0;
}

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
    for (const auto& entry : fs::directory_iterator(tmp)) {
        if (entry.is_directory()) {
            fs::path lock = entry.path() / ".autogitpull.lock";
            unsigned long pid = 0;
            if (fs::exists(lock) && read_lock_pid(lock, pid) && process_running(pid))
                add_inst(entry.path().filename().string(), pid);
        }
    }

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA("\\\\.\\pipe\\autogitpull-*", &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string pipe = ffd.cFileName;
            std::string full = "\\\\.\\pipe\\" + pipe;
            UniqueHandle h(CreateFileA(full.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                       OPEN_EXISTING, 0, nullptr));
            if (h) {
                ULONG pid = 0;
                if (GetNamedPipeServerProcessId(h.get(), &pid))
                    add_inst(pipe.substr(std::strlen("autogitpull-")),
                             static_cast<unsigned long>(pid));
            }
        } while (FindNextFileA(hFind, &ffd) != 0);
        FindClose(hFind);
    }

    UniqueHandle snap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (snap) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        if (Process32First(snap.get(), &pe)) {
            do {
#ifdef UNICODE
                std::wstring name = pe.szExeFile;
                if (name == L"autogitpull.exe" || name == L"autogitpull")
#else
                std::string name = pe.szExeFile;
                if (name == "autogitpull.exe" || name == "autogitpull")
#endif
                    add_inst("autogitpull", static_cast<unsigned long>(pe.th32ProcessID));
            } while (Process32Next(snap.get(), &pe));
        }
    }

    return out;
}

} // namespace procutil
