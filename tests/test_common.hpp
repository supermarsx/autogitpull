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
#include "lock_utils.hpp"
#include "options.hpp"
#include "scanner.hpp"
#include "ui_loop.hpp"
#include "process_monitor.hpp"
#include "thread_compat.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#endif
#if defined(__linux__) || defined(__APPLE__)
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <vector>
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
