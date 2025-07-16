#include <catch2/catch_test_macros.hpp>
#include "git_utils.hpp"
#include "repo.hpp"
#include "resource_utils.hpp"
#include <filesystem>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <fstream>
#include <cstdlib>

namespace fs = std::filesystem;

void scan_repos(const std::vector<fs::path> &all_repos, std::map<fs::path, RepoInfo> &repo_infos,
                std::set<fs::path> &skip_repos, std::mutex &mtx, std::atomic<bool> &scanning_flag,
                std::atomic<bool> &running, std::string &action, std::mutex &action_mtx,
                bool include_private, const fs::path &log_dir, bool check_only, bool hash_check,
                size_t concurrency, int cpu_percent_limit, size_t mem_limit);

TEST_CASE("scan_repos memory stability") {
    git::GitInitGuard guard;
    fs::path repo = fs::temp_directory_path() / "memory_leak_repo";
    fs::remove_all(repo);
    fs::create_directory(repo);

    REQUIRE(std::system(("git init " + repo.string() + " > /dev/null 2>&1").c_str()) == 0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system(("git -C " + repo.string() + " add file.txt").c_str());
    std::system(("git -C " + repo.string() + " commit -m init > /dev/null 2>&1").c_str());

    std::vector<fs::path> repos{repo};
    std::map<fs::path, RepoInfo> infos;
    std::set<fs::path> skip;
    std::mutex mtx;
    std::atomic<bool> scanning(false);
    std::atomic<bool> running(true);
    std::string action;
    std::mutex action_mtx;

    size_t baseline = 0;
    for (int i = 0; i < 100; ++i) {
        scanning = true;
        running = true;
        scan_repos(repos, infos, skip, mtx, scanning, running, action, action_mtx, false,
                   fs::path(), true, true, 1, 0, 0);
        size_t mem = procutil::get_memory_usage_mb();
        if (i == 0)
            baseline = mem;
        REQUIRE(mem <= baseline + 20);
    }

    fs::remove_all(repo);
}
