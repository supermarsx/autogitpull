#include "logger.hpp"
#include <fstream>
#include <mutex>
#include <iostream>
#include "time_utils.hpp"

static std::ofstream g_log_ofs;
static std::mutex g_log_mtx;
static LogLevel g_min_level = LogLevel::INFO;

void init_logger(const std::string& path, LogLevel level) {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    if (g_log_ofs.is_open()) {
        g_log_ofs.flush();
        g_log_ofs.close();
    } else {
        g_log_ofs.clear();
    }
    g_log_ofs.open(path, std::ios::app);
    if (!g_log_ofs.is_open()) {
        std::cerr << "Failed to open log file: " << path << std::endl;
        return;
    }
    g_min_level = level;
}

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
}

void log_debug(const std::string& msg) { log(LogLevel::DEBUG, "DEBUG", msg); }
void log_info(const std::string& msg) { log(LogLevel::INFO, "INFO", msg); }

void log_warning(const std::string& msg) { log(LogLevel::WARNING, "WARNING", msg); }

void log_error(const std::string& msg) { log(LogLevel::ERR, "ERROR", msg); }

static void close_logger() {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    if (g_log_ofs.is_open()) {
        g_log_ofs.flush();
        g_log_ofs.close();
    }
}

void shutdown_logger() { close_logger(); }
