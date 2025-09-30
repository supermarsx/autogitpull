#ifndef REPO_OPTIONS_HPP
#define REPO_OPTIONS_HPP

#include <optional>
#include <chrono>
#include <cstddef>
#include <filesystem>

struct RepoOptions {
    std::optional<bool> force_pull;
    std::optional<bool> exclude;
    std::optional<bool> check_only;
    std::optional<double> cpu_limit;
    std::optional<size_t> download_limit;
    std::optional<size_t> upload_limit;
    std::optional<size_t> disk_limit;
    std::optional<std::chrono::seconds> max_runtime;
    std::optional<std::chrono::seconds> pull_timeout;
    std::optional<std::filesystem::path> post_pull_hook;
    std::optional<std::string> pull_ref;
};

#endif // REPO_OPTIONS_HPP
