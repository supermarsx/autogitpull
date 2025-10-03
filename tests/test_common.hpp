#pragma once
#include <catch2/catch_test_macros.hpp>
#include "arg_parser.hpp"
#include "git_utils.hpp"
#include "repo.hpp"
#include "logger.hpp"
#include "resource_utils.hpp"
#include "time_utils.hpp"
#include "config_utils.hpp"
#include "parse_utils.hpp"
#include "history_utils.hpp"
#include "lock_utils.hpp"
#include "options.hpp"
#include "scanner.hpp"
#include "ui_loop.hpp"
#include "process_monitor.hpp"
#include "thread_compat.hpp"
#include <filesystem>
#include <fstream>
#include <system_error>
#include <cstdlib>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdlib>
// Portable wrappers for environment manipulation in tests
inline int setenv(const char* name, const char* value, int) { return _putenv_s(name, value); }
inline int unsetenv(const char* name) { return _putenv_s(name, ""); }
// Map POSIX popen/pclose to Windows equivalents for tests
#define popen _popen
#define pclose _pclose
// Redirect stdout/stderr to null device in system() invocations
#define REDIR " > NUL 2>&1"
#endif

#if !defined(REDIR)
#define REDIR " > /dev/null 2>&1"
#endif

static inline bool have_git() { return std::system("git --version " REDIR) == 0; }
#if defined(__linux__) || defined(__APPLE__)
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#ifdef __linux__
#include <sys/un.h>
#endif

namespace fs = std::filesystem;

static inline std::size_t read_thread_count() {
#ifdef __linux__
    std::ifstream status("/proc/self/status");
    std::string key;
    std::size_t val = 0;
    while (status >> key) {
        if (key == "Threads:") {
            status >> val;
            break;
        }
        std::string rest;
        std::getline(status, rest);
    }
    return val;
#else
    return procutil::get_thread_count();
#endif
}

namespace autogitpull::test_support {
namespace detail {
inline bool remove_once(const fs::path& target, bool recursive, std::error_code& ec) {
    ec.clear();
    if (recursive) {
        fs::remove_all(target, ec);
        if (!ec)
            return true;
        if (ec == std::errc::no_such_file_or_directory)
            return true;
        return false;
    }
    fs::remove(target, ec);
    if (!ec)
        return true;
    if (ec == std::errc::no_such_file_or_directory)
        return true;
    return false;
}

inline void remove_with_retry(const fs::path& target, bool recursive) {
    std::error_code ec;
#if defined(_WIN32)
    constexpr int kMaxAttempts = 10;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        if (remove_once(target, recursive, ec))
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
#else
    if (remove_once(target, recursive, ec))
        return;
#endif
    INFO("Failed to remove '" << target.string() << "': " << ec.message());
    REQUIRE(false);
}
}  // namespace detail

inline void remove_path(const fs::path& target) {
    detail::remove_with_retry(target, false);
}

inline void remove_all(const fs::path& target) {
    detail::remove_with_retry(target, true);
}
}  // namespace autogitpull::test_support

#ifndef FS_REMOVE
#define FS_REMOVE(path) ::autogitpull::test_support::remove_path((path))
#endif
#ifndef FS_REMOVE_ALL
#define FS_REMOVE_ALL(path) ::autogitpull::test_support::remove_all((path))
#endif
