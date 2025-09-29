#ifndef TIME_UTILS_HPP
#define TIME_UTILS_HPP

#include <string>
#include <chrono>

/**
 * @brief Get the current local time formatted as YYYY-MM-DD HH:MM:SS.
 */
std::string timestamp();

/**
 * @brief Format a duration in seconds as a short string like 1h2m3s.
 */
std::string format_duration_short(std::chrono::seconds dur);

#endif // TIME_UTILS_HPP
