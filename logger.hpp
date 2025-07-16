#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <string>

enum class LogLevel { DEBUG = 0, INFO, WARNING, ERROR };

void init_logger(const std::string &path, LogLevel level = LogLevel::INFO);
void set_log_level(LogLevel level);
bool logger_initialized();
void log_debug(const std::string &msg);
void log_info(const std::string &msg);
void log_warning(const std::string &msg);
void log_error(const std::string &msg);
void close_logger();

#endif // LOGGER_HPP
