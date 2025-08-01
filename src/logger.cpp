#include "logger.hpp"
#include <fstream>
#include <mutex>
#include <iostream>
#include <filesystem>
#include "time_utils.hpp"
#ifdef __linux__
#include <syslog.h>
#endif

static std::ofstream g_log_ofs;
static std::mutex g_log_mtx;
static LogLevel g_min_level = LogLevel::INFO;
static std::string g_log_path; // NOLINT(runtime/string)
static size_t g_max_size = 0;
#ifdef __linux__
static bool g_syslog = false;
static int g_facility = LOG_USER;
#endif

void init_logger(const std::string& path, LogLevel level, size_t max_size) {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    if (g_log_ofs.is_open()) {
        g_log_ofs.flush();
        g_log_ofs.close();
    } else {
        g_log_ofs.clear();
    }
    g_log_path = path;
    g_max_size = max_size;
    g_log_ofs.open(path, std::ios::app);
    if (!g_log_ofs.is_open()) {
        std::cerr << "Failed to open log file: " << path << std::endl;
        return;
    }
    g_min_level = level;
}

#ifdef __linux__
void init_syslog(int facility) {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    g_facility = facility;
    g_syslog = true;
    openlog("autogitpull", LOG_PID | LOG_CONS, facility);
}
#else
void init_syslog(int) {}
#endif

void set_log_level(LogLevel level) {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    g_min_level = level;
}

bool logger_initialized() {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    return g_log_ofs.is_open();
}

static void log(LogLevel level, const std::string& label, const std::string& msg) {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    if (!g_log_ofs.is_open() || level < g_min_level)
        return;
    g_log_ofs << "[" << timestamp() << "] [" << label << "] " << msg << std::endl;
    if (g_max_size > 0) {
        g_log_ofs.flush();
        std::error_code ec;
        if (std::filesystem::file_size(g_log_path, ec) > g_max_size && !ec) {
            g_log_ofs.close();
            g_log_ofs.open(g_log_path, std::ios::trunc);
        }
    }
#ifdef __linux__
    if (g_syslog) {
        int pri = LOG_INFO;
        switch (level) {
        case LogLevel::DEBUG:
            pri = LOG_DEBUG;
            break;
        case LogLevel::INFO:
            pri = LOG_INFO;
            break;
        case LogLevel::WARNING:
            pri = LOG_WARNING;
            break;
        case LogLevel::ERR:
            pri = LOG_ERR;
            break;
        }
        syslog(pri, "%s", msg.c_str());
    }
#endif
}

void log_debug(const std::string& msg) { log(LogLevel::DEBUG, "DEBUG", msg); }
void log_info(const std::string& msg) { log(LogLevel::INFO, "INFO", msg); }

void log_warning(const std::string& msg) { log(LogLevel::WARNING, "WARNING", msg); }

void log_error(const std::string& msg) { log(LogLevel::ERR, "ERROR", msg); }

#ifdef __linux__
static void close_logger() {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    if (g_log_ofs.is_open()) {
        g_log_ofs.flush();
        g_log_ofs.close();
    }
    if (g_syslog)
        closelog();
}
#else
static void close_logger() {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    if (g_log_ofs.is_open()) {
        g_log_ofs.flush();
        g_log_ofs.close();
    }
}
#endif

void shutdown_logger() { close_logger(); }
