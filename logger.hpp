#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <string>

enum class LogLevel {
    Error = 0,
    Warning,
    Info
};

void init_logger(const std::string& path, LogLevel level = LogLevel::Info);
bool logger_initialized();
void log_info(const std::string& msg);
void log_warning(const std::string& msg);
void log_error(const std::string& msg);

#endif // LOGGER_HPP
