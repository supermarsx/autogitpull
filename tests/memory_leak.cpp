#include <cstdlib>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <catch2/catch_test_macros.hpp>
#include "test_common.hpp"
#include "git_utils.hpp"
#include "repo.hpp"
#include "resource_utils.hpp"
#include "scanner.hpp"

namespace fs = std::filesystem;

TEST_CASE("scan_repos memory stability") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path repo = fs::temp_directory_path() / "memory_leak_repo";
    FS_REMOVE_ALL(repo);
    fs::create_directory(repo);

    REQUIRE(std::system(("git init " + repo.string() + REDIR).c_str()) == 0);
    (void)std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    (void)std::system(("git -C " + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    (void)std::system(("git -C " + repo.string() + " add file.txt").c_str());
    (void)std::system(("git -C " + repo.string() + " commit -m init" REDIR).c_str());

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
        scan_repos(repos, infos, skip, mtx, scanning, running, action, action_mtx, false, "origin",
                   fs::path(), true, true, 1, 0, 0, 0, 0, 0, false, false, false, false, false,
                   false, true, fs::path(), std::chrono::seconds(0), false, std::chrono::seconds(0),
                   false, false, {}, false);
        size_t mem = procutil::get_memory_usage_mb();
        if (i == 0)
            baseline = mem;
        REQUIRE(mem <= baseline + 20);
    }

    FS_REMOVE_ALL(repo);
}
