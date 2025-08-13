#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <string>

enum class LogLevel { DEBUG = 0, INFO, WARNING, ERR };

void init_logger(const std::string& path, LogLevel level = LogLevel::INFO, size_t max_size = 0);
void set_log_level(LogLevel level);
void set_json_logging(bool enable);
bool logger_initialized();
void log_event(LogLevel level, const std::string& message);
void log_event(LogLevel level, const std::string& message, const std::string& data);
void log_debug(const std::string& msg);
void log_debug(const std::string& msg, const std::string& data);
void log_info(const std::string& msg);
void log_info(const std::string& msg, const std::string& data);
void log_warning(const std::string& msg);
void log_warning(const std::string& msg, const std::string& data);
void log_error(const std::string& msg);
void log_error(const std::string& msg, const std::string& data);
void init_syslog(int facility = 0);
void shutdown_logger();

#endif // LOGGER_HPP
