#include "scanner.hpp"

#include <filesystem>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#else
#include <process.h>
#endif

namespace fs = std::filesystem;

void run_post_pull_hook(const fs::path& hook) {
    if (hook.empty())
        return;
#ifdef _WIN32
    _wspawnl(_P_WAIT, hook.wstring().c_str(), hook.wstring().c_str(), nullptr);
#else
    pid_t pid = fork();
    if (pid == 0) {
        execl(hook.c_str(), hook.c_str(), (char*)nullptr);
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    }
#endif
}
