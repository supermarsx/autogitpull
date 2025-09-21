#ifndef LOCK_UTILS_HPP
#define LOCK_UTILS_HPP
#include <filesystem>
#include <vector>
#include <string>
#include <utility>

namespace procutil {

/**
 * @brief Attempt to acquire an exclusive lock by creating a lock file.
 *
 * The current process ID is written into the file so other processes can
 * determine who holds the lock.
 *
 * @param path Filesystem location of the lock file.
 * @return true if the lock file was successfully created.
 */
bool acquire_lock_file(const std::filesystem::path& path);

/**
 * @brief Release a previously acquired lock file.
 *
 * The lock is released by deleting the file at @a path.
 *
 * @param path Filesystem location of the lock file.
 */
void release_lock_file(const std::filesystem::path& path);

/**
 * @brief Read the PID stored in a lock file.
 *
 * @param path Path to the lock file.
 * @param pid  Output variable receiving the parsed process ID.
 * @return true if the PID was successfully read.
 */
bool read_lock_pid(const std::filesystem::path& path, unsigned long& pid);

/**
 * @brief Check whether a process with the given PID is currently running.
 *
 * @param pid Process identifier to query.
 * @return true if the process exists and is active.
 */
bool process_running(unsigned long pid);

/**
 * @brief Attempt to terminate the process identified by @a pid.
 *
 * @param pid Process identifier to terminate.
 * @return true if the termination signal was successfully sent.
 */
bool terminate_process(unsigned long pid);

/**
 * @brief Find other running instances of the application.
 *
 * Searches lock files, sockets, and process listings to discover existing
 * autogitpull processes.
 *
 * @return Vector of pairs containing instance names and their PIDs.
 */
std::vector<std::pair<std::string, unsigned long>> find_running_instances();

/**
 * @brief RAII guard that holds a lock file for its lifetime.
 *
 * The constructor attempts to acquire the lock at the given path and the
 * destructor releases it if held, ensuring single-instance semantics.
 */
struct LockFileGuard {
    std::filesystem::path path; ///< Location of the lock file.
    bool locked = false;        ///< Whether the lock was successfully acquired.
    explicit LockFileGuard(const std::filesystem::path& p); ///< Acquire lock.
    ~LockFileGuard();                                       ///< Release lock.
    LockFileGuard(const LockFileGuard&) = delete;
    LockFileGuard& operator=(const LockFileGuard&) = delete;
};

} // namespace procutil

#endif // LOCK_UTILS_HPP
