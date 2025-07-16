#ifndef REPO_HPP
#define REPO_HPP
#include <filesystem>
#include <string>

/**
 * @brief High level status for a repository being monitored.
 */
enum RepoStatus {
    RS_PENDING,       ///< Repository has not been processed yet
    RS_CHECKING,      ///< Currently checking remote state
    RS_UP_TO_DATE,    ///< Local repository is up to date
    RS_PULLING,       ///< Pull operation in progress
    RS_PULL_OK,       ///< Pull completed successfully
    RS_PKGLOCK_FIXED, ///< Special case: package-lock.json reset
    RS_ERROR,         ///< An error occurred while processing
    RS_SKIPPED,       ///< Repository was skipped
    RS_HEAD_PROBLEM,  ///< HEAD/branch mismatch detected
    RS_REMOTE_AHEAD   ///< Remote contains newer commits
};

/**
 * @brief Runtime information about a repository.
 */
struct RepoInfo {
    std::filesystem::path path;     ///< Filesystem location of the repository
    RepoStatus status = RS_PENDING; ///< Current status code
    std::string message;            ///< Human readable status message
    std::string branch;             ///< Currently checked-out branch
    std::string commit;             ///< Short hash of HEAD
    std::string last_pull_log;      ///< Result of last pull attempt
    int progress = 0;               ///< Fetch progress percentage
    bool auth_failed = false;       ///< Authentication error flag
};

#endif // REPO_HPP
