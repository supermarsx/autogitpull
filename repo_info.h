#ifndef REPO_INFO_H
#define REPO_INFO_H

#include <filesystem>
#include <string>

namespace git {
namespace fs = std::filesystem;

enum RepoStatus {
    RS_CHECKING,
    RS_UP_TO_DATE,
    RS_PULLING,
    RS_PULL_OK,
    RS_PKGLOCK_FIXED,
    RS_ERROR,
    RS_SKIPPED,
    RS_HEAD_PROBLEM
};

struct RepoInfo {
    fs::path path;
    RepoStatus status = RS_CHECKING;
    std::string message;
    std::string branch;
    std::string last_pull_log;
};

} // namespace git

#endif // REPO_INFO_H
