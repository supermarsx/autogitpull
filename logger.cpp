#include "logger.hpp"
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>

static std::ofstream g_log_ofs;
static std::mutex g_log_mtx;
static LogLevel g_level = LogLevel::Info;

void init_logger(const std::string& path, LogLevel level) {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    g_log_ofs.open(path, std::ios::app);
    g_level = level;
}

bool logger_initialized() {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    return g_log_ofs.is_open();
}

static std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

static void log(LogLevel lvl, const char* label, const std::string& msg) {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    if (!g_log_ofs.is_open()) return;
    if (static_cast<int>(lvl) > static_cast<int>(g_level)) return;
    g_log_ofs << "[" << timestamp() << "] [" << label << "] " << msg << std::endl;
}

void log_info(const std::string& msg) {
    log(LogLevel::Info, "INFO", msg);
}

void log_warning(const std::string& msg) {
    log(LogLevel::Warning, "WARN", msg);
}

void log_error(const std::string& msg) {
    log(LogLevel::Error, "ERROR", msg);
}
