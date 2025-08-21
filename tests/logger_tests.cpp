#include <cstdarg>
#include <cstdio>
#ifdef __linux__
#include <syslog.h>
#endif
#include <fstream>
#include <string>
#include <vector>
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
    REQUIRE(logger_initialized());
    for (int i = 0; i < 200; ++i)
        log_info("entry " + std::to_string(i));
    shutdown_logger();

    REQUIRE(std::filesystem::exists(log));
    REQUIRE(std::filesystem::exists(log1));
    REQUIRE(std::filesystem::exists(log2));
    REQUIRE_FALSE(std::filesystem::exists(log3));

    fs::remove(log);
    fs::remove(log1);
    fs::remove(log2);
}

TEST_CASE("Logger switches between JSON and plain") {
    fs::path log = fs::temp_directory_path() / "logger_format.log";
    fs::remove(log);
    init_logger(log.string());
    set_json_logging(true);
    log_info("json entry", {{"k", "v"}});
    set_json_logging(false);
    log_info("plain entry");
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
