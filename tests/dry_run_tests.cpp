#include "test_common.hpp"

TEST_CASE("dry-run skips pulls") {
    git::GitInitGuard guard;
    fs::path repo = fs::temp_directory_path() / "dry_run_repo";
    fs::remove_all(repo);
    fs::create_directory(repo);
    REQUIRE(std::system(("git init " + repo.string() + " > /dev/null 2>&1").c_str()) == 0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system(("git -C " + repo.string() + " add file.txt").c_str());
    std::system(("git -C " + repo.string() + " commit -m init > /dev/null 2>&1").c_str());
    std::system(("git -C " + repo.string() + " remote add origin https://github.com/octocat/Hello-World.git").c_str());

    std::vector<fs::path> repos{repo};
    std::map<fs::path, RepoInfo> infos;
    std::set<fs::path> skip;
    std::mutex mtx;
    std::atomic<bool> scanning(false);
    std::atomic<bool> running(true);
    std::string action;
    std::mutex action_mtx;
    scan_repos(repos, infos, skip, mtx, scanning, running, action, action_mtx, false,
               "origin", fs::path(), false, false, 1, 0, 0, 0, 0, 0, false, false, false,
               std::chrono::seconds(0), false, std::chrono::seconds(0), true, false, false,
               {}, false);
    REQUIRE(infos[repo].status == RS_SKIPPED);
    REQUIRE(infos[repo].message == "Dry run");
    fs::remove_all(repo);
}
