#include "logger.hpp"
#include <zlib.h>
#include <fstream>
#include <mutex>
#include <iostream>
#include <filesystem>
#include <map>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <condition_variable>
#include <queue>
#include "time_utils.hpp"
#ifdef __linux__
#include <syslog.h>
#endif

static std::ofstream g_log_ofs;
static std::string g_log_path; // NOLINT(runtime/string)
static std::atomic<LogLevel> g_min_level{LogLevel::INFO};
static std::atomic<size_t> g_max_size{0};
static std::atomic<size_t> g_max_files{1};
static std::atomic<bool> g_json_log{false};
static std::atomic<bool> g_compress_logs{false};
#ifdef __linux__
static std::atomic<bool> g_syslog{false};
static std::atomic<int> g_facility{LOG_USER};
#endif

struct LogMessage {
    LogLevel level;
    std::string label;
    std::string msg;
    std::map<std::string, std::string> fields;
};

static std::queue<LogMessage*> g_log_queue;
static std::mutex g_queue_mtx;
static std::condition_variable g_queue_cv;
static std::atomic<bool> g_running{false};
static std::thread g_log_thread;
static std::mutex g_init_mtx;

static void log_worker();
static void stop_log_thread() {
    {
        std::lock_guard<std::mutex> qlk(g_queue_mtx);
        g_running.store(false);
    }
    g_queue_cv.notify_all();
    if (g_log_thread.joinable())
        g_log_thread.join();
}

/**
 * @brief Initialize file-based logging.
 *
 * Opens @p path for append, sets the minimum @ref LogLevel, and
 * configures size-based log rotation.
 *
 * @param path      Filesystem location of the log file.
 * @param level     Minimum severity to record.
 * @param max_size  Maximum file size in bytes before rotation.
 * @param max_files Number of rotated files to retain.
 */
void init_logger(const std::string& path, LogLevel level, size_t max_size, size_t max_files) {
    std::lock_guard<std::mutex> lk(g_init_mtx);
    std::string prev_path = g_log_path;
    stop_log_thread();
    if (g_log_ofs.is_open()) {
        g_log_ofs.flush();
        g_log_ofs.close();
    } else {
        g_log_ofs.clear();
    }
    g_max_size.store(max_size);
    g_max_files.store(max_files);
    std::string target = path;
    g_log_ofs.open(target, std::ios::app);
    if (!g_log_ofs.is_open()) {
        std::cerr << "Failed to open log file: " << path << std::endl;
        target = prev_path;
        if (!target.empty())
            g_log_ofs.open(target, std::ios::app);
    }
    g_log_path = target;
    g_min_level.store(level);
    g_running.store(true);
    g_log_thread = std::thread(log_worker);
}

#ifdef __linux__
/**
 * @brief Enable syslog integration.
 *
 * Configures the syslog facility and opens a connection so future
 * messages are mirrored to the system log.
 *
 * @param facility Syslog facility identifier.
 */
void init_syslog(int facility) {
    std::lock_guard<std::mutex> lk(g_init_mtx);
    g_facility.store(facility);
    g_syslog.store(true);
    openlog("autogitpull", LOG_PID | LOG_CONS, facility);
}
#else
void init_syslog(int) {}
#endif

/**
 * @brief Set the minimum log level.
 *
 * Adjusts the severity threshold below which messages are discarded.
 *
 * @param level New minimum @ref LogLevel.
 */
void set_log_level(LogLevel level) { g_min_level.store(level); }

/**
 * @brief Toggle JSON formatted logging.
 *
 * When enabled, log entries are serialized as JSON objects instead of
 * plain text.
 *
 * @param enable `true` to emit JSON logs.
 */
void set_json_logging(bool enable) { g_json_log.store(enable); }

void set_log_compression(bool enable) { g_compress_logs.store(enable); }

void set_log_rotation(size_t max_files) { g_max_files.store(max_files); }

bool logger_initialized() {
    std::lock_guard<std::mutex> lk(g_init_mtx);
    return g_log_ofs.is_open();
}

void flush_logger() {
    std::unique_lock<std::mutex> lk(g_queue_mtx);
    while (!g_log_queue.empty()) {
        lk.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        lk.lock();
    }
    g_log_ofs.flush();
}

static bool gzip_file(const std::string& src, const std::string& dst) {
    std::ifstream in(src, std::ios::binary);
    gzFile out = gzopen(dst.c_str(), "wb");
    if (!in.is_open() || out == nullptr) {
        if (out)
            gzclose(out);
        return false;
    }
    char buf[8192];
    while (in) {
        in.read(buf, sizeof(buf));
        std::streamsize n = in.gcount();
        if (n > 0)
            gzwrite(out, buf, static_cast<unsigned int>(n));
    }
    gzclose(out);
    return true;
}

static std::string json_escape(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

static std::string format_extra_json(const std::map<std::string, std::string>& fields) {
    if (fields.empty())
        return "";
    std::string out;
    bool first = true;
    for (const auto& [k, v] : fields) {
        if (!first)
            out += ",";
        out += "\"" + json_escape(k) + "\":\"" + json_escape(v) + "\"";
        first = false;
    }
    return out;
}

/**
 * @brief Core logging routine.
 *
 * Formats the message, writes it to the file sink, and optionally
 * forwards it to syslog. Supports structured fields and JSON output.
 *
 * @param level  Severity of the message.
 * @param label  Text label for the level (e.g., "INFO").
 * @param msg    Human-readable message body.
 * @param fields Additional key/value fields to serialize.
 */
static void write_log_entry(LogLevel level, const std::string& label, const std::string& msg,
                            const std::map<std::string, std::string>& fields) {
    if (!g_log_ofs.is_open() || level < g_min_level.load())
        return;
    std::string line;
    std::string ts = timestamp();
    if (g_json_log.load()) {
        // Escape special characters so the output remains valid JSON.
        line = "{\"timestamp\":\"" + json_escape(ts) + "\",\"level\":\"" + label + "\",\"msg\":\"" +
               json_escape(msg) + "\"";
        std::string extras = format_extra_json(fields);
        if (!extras.empty())
            line += "," + extras;
        line += "}";
    } else {
        line = "[" + ts + "] [" + label + "] " + msg;
        for (const auto& [k, v] : fields)
            line += " " + k + "=" + v;
    }
    g_log_ofs << line << std::endl;
    if (g_max_size.load() > 0) {
        // Check the rotation threshold and rotate files when the active log
        // exceeds @p g_max_size bytes.
        g_log_ofs.flush();
        std::error_code ec;
        if (std::filesystem::file_size(g_log_path, ec) > g_max_size.load() && !ec) {
            g_log_ofs.close();
            if (g_max_files.load() > 0) {
                namespace fs = std::filesystem;
                if (g_compress_logs.load()) {
                    for (size_t i = g_max_files.load(); i > 0; --i) {
                        fs::path src = g_log_path + "." + std::to_string(i) + ".gz";
                        if (i == g_max_files) {
                            fs::remove(src, ec);
                        } else {
                            fs::path dst = g_log_path + "." + std::to_string(i + 1) + ".gz";
                            fs::rename(src, dst, ec);
                        }
                    }
                    fs::path first = g_log_path + ".1";
                    fs::rename(g_log_path, first, ec);
                    fs::path gz = first;
                    gz += ".gz";
                    if (gzip_file(first.string(), gz.string()))
                        fs::remove(first, ec);
                } else {
                    for (size_t i = g_max_files.load(); i > 0; --i) {
                        fs::path src = g_log_path + "." + std::to_string(i);
                        if (i == g_max_files) {
                            fs::remove(src, ec);
                        } else {
                            fs::path dst = g_log_path + "." + std::to_string(i + 1);
                            fs::rename(src, dst, ec);
                        }
                    }
                    fs::path first = g_log_path + ".1";
                    fs::rename(g_log_path, first, ec);
                }
            }
            g_log_ofs.open(g_log_path, std::ios::trunc);
        }
    }
#ifdef __linux__
    if (g_syslog.load()) {
        // Mirror the message to syslog using the selected facility.
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
        syslog(pri, "%s", line.c_str());
    }
#endif
}

static void enqueue_message(LogLevel level, const std::string& label, const std::string& msg,
                            const std::map<std::string, std::string>& fields) {
    auto* entry = new LogMessage{level, label, msg, fields};
    {
        std::lock_guard<std::mutex> lk(g_queue_mtx);
        g_log_queue.push(entry);
    }
    g_queue_cv.notify_one();
}

static void log(LogLevel level, const std::string& label, const std::string& msg) {
    if (level < g_min_level.load())
        return;
    enqueue_message(level, label, msg, {});
}

static void log(LogLevel level, const std::string& label, const std::string& msg,
                const std::string& data) {
    if (data.empty()) {
        log(level, label, msg);
    } else {
        enqueue_message(level, label, msg, std::map<std::string, std::string>{{"data", data}});
    }
}

static void log(LogLevel level, const std::string& label, const std::string& msg,
                const std::map<std::string, std::string>& fields) {
    if (level < g_min_level.load())
        return;
    enqueue_message(level, label, msg, fields);
}

void log_event(LogLevel level, const std::string& message) {
    const char* label = "INFO";
    switch (level) {
    case LogLevel::DEBUG:
        label = "DEBUG";
        break;
    case LogLevel::INFO:
        label = "INFO";
        break;
    case LogLevel::WARNING:
        label = "WARNING";
        break;
    case LogLevel::ERR:
        label = "ERROR";
        break;
    }
    log(level, label, message);
}

void log_event(LogLevel level, const std::string& message, const std::string& data) {
    const char* label = "INFO";
    switch (level) {
    case LogLevel::DEBUG:
        label = "DEBUG";
        break;
    case LogLevel::INFO:
        label = "INFO";
        break;
    case LogLevel::WARNING:
        label = "WARNING";
        break;
    case LogLevel::ERR:
        label = "ERROR";
        break;
    }
    log(level, label, message, data);
}

void log_event(LogLevel level, const std::string& message,
               const std::map<std::string, std::string>& fields) {
    const char* label = "INFO";
    switch (level) {
    case LogLevel::DEBUG:
        label = "DEBUG";
        break;
    case LogLevel::INFO:
        label = "INFO";
        break;
    case LogLevel::WARNING:
        label = "WARNING";
        break;
    case LogLevel::ERR:
        label = "ERROR";
        break;
    }
    log(level, label, message, fields);
}

void log_debug(const std::string& msg) { log(LogLevel::DEBUG, "DEBUG", msg); }
void log_debug(const std::string& msg, const std::string& data) {
    log(LogLevel::DEBUG, "DEBUG", msg, data);
}
void log_debug(const std::string& msg, const std::map<std::string, std::string>& fields) {
    log(LogLevel::DEBUG, "DEBUG", msg, fields);
}
void log_info(const std::string& msg) { log(LogLevel::INFO, "INFO", msg); }
void log_info(const std::string& msg, const std::string& data) {
    log(LogLevel::INFO, "INFO", msg, data);
}
void log_info(const std::string& msg, const std::map<std::string, std::string>& fields) {
    log(LogLevel::INFO, "INFO", msg, fields);
}

void log_warning(const std::string& msg) { log(LogLevel::WARNING, "WARNING", msg); }
void log_warning(const std::string& msg, const std::string& data) {
    log(LogLevel::WARNING, "WARNING", msg, data);
}
void log_warning(const std::string& msg, const std::map<std::string, std::string>& fields) {
    log(LogLevel::WARNING, "WARNING", msg, fields);
}

void log_error(const std::string& msg) { log(LogLevel::ERR, "ERROR", msg); }
void log_error(const std::string& msg, const std::string& data) {
    log(LogLevel::ERR, "ERROR", msg, data);
}
void log_error(const std::string& msg, const std::map<std::string, std::string>& fields) {
    log(LogLevel::ERR, "ERROR", msg, fields);
}

static void log_worker() {
    std::vector<LogMessage*> batch;
    batch.reserve(16);
    while (true) {
        std::unique_lock<std::mutex> lk(g_queue_mtx);
        g_queue_cv.wait(lk, [] { return !g_log_queue.empty() || !g_running.load(); });
        if (!g_running.load() && g_log_queue.empty())
            break;
        while (!g_log_queue.empty() && batch.size() < 16) {
            batch.push_back(g_log_queue.front());
            g_log_queue.pop();
        }
        lk.unlock();
        for (LogMessage* m : batch) {
            write_log_entry(m->level, m->label, m->msg, m->fields);
            delete m;
        }
        batch.clear();
        g_log_ofs.flush();
    }
    g_log_ofs.flush();
}

void shutdown_logger() {
    std::lock_guard<std::mutex> lk(g_init_mtx);
    stop_log_thread();
    if (g_log_ofs.is_open()) {
        g_log_ofs.flush();
        g_log_ofs.close();
    }
#ifdef __linux__
    if (g_syslog.load()) {
        closelog();
        g_syslog.store(false);
    }
#endif
    std::lock_guard<std::mutex> qlk(g_queue_mtx);
    while (!g_log_queue.empty()) {
        delete g_log_queue.front();
        g_log_queue.pop();
    }
}
