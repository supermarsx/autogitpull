#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <string>

void init_logger(const std::string& path);
bool logger_initialized();
void log_info(const std::string& msg);
void log_error(const std::string& msg);

#endif // LOGGER_HPP
