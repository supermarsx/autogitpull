#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <string>
#include <map>

enum class LogLevel { DEBUG = 0, INFO, WARNING, ERR };

/**
 * @brief Initialize the file logger.
 *
 * Opens the log file at @p path and configures log rotation parameters.
 *
 * @param path      Filesystem path where the log file will be written.
 * @param level     Minimum @ref LogLevel severity to record.
 * @param max_size  Maximum size in bytes before rotating the file. A value of
 *                  `0` disables size-based rotation.
 * @param max_files Number of rotated log files to keep.
 */
void init_logger(const std::string& path, LogLevel level = LogLevel::INFO, size_t max_size = 0,
                 size_t max_files = 1);

/**
 * @brief Set the global minimum log level.
 *
 * @param level Desired @ref LogLevel threshold for emitting messages.
 */
void set_log_level(LogLevel level);

/**
 * @brief Enable or disable JSON formatted logging.
 *
 * @param enable Set to `true` to emit logs as JSON objects instead of plain
 *               text.
 */
void set_json_logging(bool enable);

/**
 * @brief Configure how many rotated log files are retained.
 *
 * @param max_files Number of historical log files to keep after rotation.
 */
void set_log_rotation(size_t max_files);

/**
 * @brief Check whether the logger has been initialized.
 *
 * @return `true` if the logger is ready to use; `false` otherwise.
 */
bool logger_initialized();

/**
 * @brief Log a message with the specified severity.
 *
 * @param level   Severity level for the event.
 * @param message Human-readable text describing the event.
 */
void log_event(LogLevel level, const std::string& message);

/**
 * @brief Log a message with an additional serialized payload.
 *
 * @param level   Severity level for the event.
 * @param message Human-readable text describing the event.
 * @param data    Additional string data to attach to the log entry.
 */
void log_event(LogLevel level, const std::string& message, const std::string& data);

/**
 * @brief Log a message with structured key/value fields.
 *
 * @param level   Severity level for the event.
 * @param message Human-readable text describing the event.
 * @param fields  Map of field names to values providing structured context.
 */
void log_event(LogLevel level, const std::string& message,
               const std::map<std::string, std::string>& fields);

/**
 * @brief Convenience wrapper for a debug-level log message.
 *
 * @param msg Text to record at the debug level.
 */
void log_debug(const std::string& msg);

/**
 * @brief Debug-level log with additional serialized data.
 *
 * @param msg  Text to record.
 * @param data Extra string payload to include.
 */
void log_debug(const std::string& msg, const std::string& data);

/**
 * @brief Debug-level log with structured fields.
 *
 * @param msg    Text to record.
 * @param fields Map of field names to values providing structured context.
 */
void log_debug(const std::string& msg, const std::map<std::string, std::string>& fields);

/**
 * @brief Convenience wrapper for an info-level log message.
 *
 * @param msg Text to record at the info level.
 */
void log_info(const std::string& msg);

/**
 * @brief Info-level log with additional serialized data.
 *
 * @param msg  Text to record.
 * @param data Extra string payload to include.
 */
void log_info(const std::string& msg, const std::string& data);

/**
 * @brief Info-level log with structured fields.
 *
 * @param msg    Text to record.
 * @param fields Map of field names to values providing structured context.
 */
void log_info(const std::string& msg, const std::map<std::string, std::string>& fields);

/**
 * @brief Convenience wrapper for a warning-level log message.
 *
 * @param msg Text to record at the warning level.
 */
void log_warning(const std::string& msg);

/**
 * @brief Warning-level log with additional serialized data.
 *
 * @param msg  Text to record.
 * @param data Extra string payload to include.
 */
void log_warning(const std::string& msg, const std::string& data);

/**
 * @brief Warning-level log with structured fields.
 *
 * @param msg    Text to record.
 * @param fields Map of field names to values providing structured context.
 */
void log_warning(const std::string& msg, const std::map<std::string, std::string>& fields);

/**
 * @brief Convenience wrapper for an error-level log message.
 *
 * @param msg Text to record at the error level.
 */
void log_error(const std::string& msg);

/**
 * @brief Error-level log with additional serialized data.
 *
 * @param msg  Text to record.
 * @param data Extra string payload to include.
 */
void log_error(const std::string& msg, const std::string& data);

/**
 * @brief Error-level log with structured fields.
 *
 * @param msg    Text to record.
 * @param fields Map of field names to values providing structured context.
 */
void log_error(const std::string& msg, const std::map<std::string, std::string>& fields);

/**
 * @brief Initialize system logging using the specified facility.
 *
 * @param facility Syslog facility identifier to tag messages with.
 */
void init_syslog(int facility = 0);

/**
 * @brief Shut down the logging subsystem and release resources.
 */
void shutdown_logger();

#endif // LOGGER_HPP
