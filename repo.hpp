#ifndef REPO_HPP
#define REPO_HPP
#include <filesystem>
#include <string>

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
    std::filesystem::path path;
    RepoStatus status = RS_CHECKING;
    std::string message;
    std::string branch;
    std::string last_pull_log;
    int progress = 0; // fetch progress percentage
};

#endif // REPO_HPP
