#include <zlib.h>
#include <cstdarg>
#include <cstdio>
#ifdef __linux__
#include <syslog.h>
#endif
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>
#include <future>
#include "test_common.hpp"
#ifdef __linux__
static std::vector<std::string> g_syslog_messages;
extern "C" void openlog(const char*, int, int) {}
extern "C" void syslog(int, const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    g_syslog_messages.emplace_back(buf);
}
extern "C" void closelog() {}
#endif

struct LoggerGuard {
    ~LoggerGuard() { shutdown_logger(); }
};

TEST_CASE("Logger rotates and limits files") {
    fs::path log = fs::temp_directory_path() / "logger_rotate.log";
    fs::path log1 = log;
    log1 += ".1";
    fs::path log2 = log;
    log2 += ".2";
    fs::path log3 = log;
    log3 += ".3";
    fs::remove(log);
    fs::remove(log1);
    fs::remove(log2);
    fs::remove(log3);

    init_logger(log.string(), LogLevel::INFO, 100, 2);
    LoggerGuard guard;
    REQUIRE(logger_initialized());
    for (int i = 0; i < 200; ++i)
        log_info("entry " + std::to_string(i));
    flush_logger();
    shutdown_logger();
    REQUIRE(std::filesystem::exists(log));
    REQUIRE(std::filesystem::exists(log1));
    REQUIRE(std::filesystem::exists(log2));
    REQUIRE_FALSE(std::filesystem::exists(log3));

    fs::remove(log);
    fs::remove(log1);
    fs::remove(log2);
}

TEST_CASE("Logger compresses rotated files") {
    fs::path log = fs::temp_directory_path() / "logger_compress.log";
    fs::path log1 = log;
    log1 += ".1.gz";
    fs::path log2 = log;
    log2 += ".2.gz";
    fs::remove(log);
    fs::remove(log1);
    fs::remove(log2);

    set_log_compression(true);
    init_logger(log.string(), LogLevel::INFO, 100, 2);
    LoggerGuard guard;
    for (int i = 0; i < 200; ++i)
        log_info("entry " + std::to_string(i));
    shutdown_logger();

    REQUIRE(std::filesystem::exists(log));
    REQUIRE(std::filesystem::exists(log1));
    REQUIRE(std::filesystem::exists(log2));

    gzFile zf = gzopen(log1.c_str(), "rb");
    REQUIRE(zf != nullptr);
    char buf[32];
    int n = gzread(zf, buf, sizeof(buf));
    gzclose(zf);
    REQUIRE(n > 0);

    fs::remove(log);
    fs::remove(log1);
    fs::remove(log2);
    set_log_compression(false);
}

TEST_CASE("Logger switches between JSON and plain") {
    fs::path log = fs::temp_directory_path() / "logger_format.log";
    fs::remove(log);
    init_logger(log.string());
    LoggerGuard guard;
    set_json_logging(true);
    log_info("json entry", {{"k", "v"}});
    flush_logger();
    set_json_logging(false);
    log_info("plain entry");
    flush_logger();
    shutdown_logger();

    std::ifstream ifs(log);
    REQUIRE(ifs.good());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);
    REQUIRE(lines.size() == 2);
    REQUIRE(!lines[0].empty());
    REQUIRE(lines[0][0] == '{');
    REQUIRE(lines[0].find("\"k\":\"v\"") != std::string::npos);
    REQUIRE(!lines[1].empty());
    REQUIRE(lines[1][0] == '[');

    fs::remove(log);
}

TEST_CASE("shutdown_logger drains queued messages") {
    fs::path log = fs::temp_directory_path() / "logger_drain.log";
    fs::remove(log);
    init_logger(log.string());
    for (int i = 0; i < 50; ++i)
        log_info("queued " + std::to_string(i));
    shutdown_logger();
    std::ifstream ifs(log);
    REQUIRE(ifs.good());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);
    REQUIRE(lines.size() == 50);
    fs::remove(log);
}

TEST_CASE("shutdown_logger exits cleanly with no messages") {
    fs::path log = fs::temp_directory_path() / "logger_noop.log";
    fs::remove(log);
    init_logger(log.string());
    LoggerGuard guard;
    REQUIRE(logger_initialized());
    shutdown_logger();
    REQUIRE_FALSE(logger_initialized());
    REQUIRE(fs::exists(log));
    REQUIRE(fs::file_size(log) == 0);
    fs::remove(log);
}

TEST_CASE("init_logger can be called twice") {
    fs::path log = fs::temp_directory_path() / "logger_reinit.log";
    fs::remove(log);
    init_logger(log.string());
    log_info("first entry");
    init_logger(log.string());
    log_info("second entry");
    shutdown_logger();
    std::ifstream ifs(log);
    REQUIRE(ifs.good());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);
    REQUIRE(lines.size() >= 2);
    fs::remove(log);
}

TEST_CASE("init_logger preserves queued messages during reinit") {
    fs::path log = fs::temp_directory_path() / "logger_reinit_queue.log";
    fs::remove(log);
    init_logger(log.string());
    std::atomic<bool> run{true};
    std::atomic<int> produced{0};
    std::thread t([&] {
        while (run.load()) {
            log_info("entry " + std::to_string(produced.fetch_add(1)));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    init_logger(log.string());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    run.store(false);
    t.join();
    shutdown_logger();
    std::ifstream ifs(log);
    REQUIRE(ifs.good());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);
    REQUIRE(lines.size() >= static_cast<size_t>(produced.load()));
    fs::remove(log);
}

TEST_CASE("init_logger skips worker when file cannot be opened") {
    fs::path good = fs::temp_directory_path() / "logger_skip_worker.log";
    fs::path bad = good.parent_path() / "missing" / "logger.log";
    fs::remove(good);
    init_logger(bad.string());
    log_info("queued");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    init_logger(good.string());
    shutdown_logger();
    std::ifstream ifs(good);
    REQUIRE(ifs.good());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);
    REQUIRE(lines.size() == 1);
    REQUIRE(lines[0].find("queued") != std::string::npos);
    fs::remove(good);
}

TEST_CASE("init_logger restores thread on failed reopen") {
    fs::path log = fs::temp_directory_path() / "logger_fail_reinit.log";
    fs::remove(log);
    init_logger(log.string());
    log_info("before");
    fs::path bad = log.parent_path() / "missing" / "logger.log";
    init_logger(bad.string());
    log_info("after");
    shutdown_logger();
    std::ifstream ifs(log);
    REQUIRE(ifs.good());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);
    REQUIRE(lines.size() >= 2);
    fs::remove(log);
}

TEST_CASE("init_logger and shutdown_logger can run concurrently") {
    fs::path log1 = fs::temp_directory_path() / "logger_race1.log";
    fs::path log2 = fs::temp_directory_path() / "logger_race2.log";
    fs::remove(log1);
    fs::remove(log2);
    init_logger(log1.string());
    std::promise<void> go;
    auto ready = go.get_future().share();
    std::thread t1([&] {
        ready.wait();
        init_logger(log2.string());
    });
    std::thread t2([&] {
        ready.wait();
        shutdown_logger();
    });
    go.set_value();
    t1.join();
    t2.join();
    if (logger_initialized())
        shutdown_logger();
    fs::remove(log1);
    fs::remove(log2);
    REQUIRE(true);
}

#ifdef __linux__
TEST_CASE("init_syslog routes messages") {
    fs::path log = fs::temp_directory_path() / "logger_syslog.log";
    fs::remove(log);
    g_syslog_messages.clear();
    init_logger(log.string());
    init_syslog(LOG_USER);
    log_info("syslog entry");
    shutdown_logger();
    REQUIRE_FALSE(g_syslog_messages.empty());
    REQUIRE(g_syslog_messages.back().find("syslog entry") != std::string::npos);
    fs::remove(log);
}
#endif
