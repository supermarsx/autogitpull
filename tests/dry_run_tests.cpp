#include "test_common.hpp"

namespace fs = std::filesystem;

TEST_CASE("scan_repos honors dry run") {
    git::GitInitGuard guard;
    fs::path remote = fs::temp_directory_path() / "dry_remote.git";
    fs::path src = fs::temp_directory_path() / "dry_src";
    fs::path repo = fs::temp_directory_path() / "dry_repo";
    fs::remove_all(remote);
    fs::remove_all(src);
    fs::remove_all(repo);

    REQUIRE(std::system(("git init --bare " + remote.string() + " > /dev/null 2>&1").c_str()) == 0);
    REQUIRE(std::system(("git clone " + remote.string() + " " + src.string() + " > /dev/null 2>&1").c_str()) == 0);
    std::system(("git -C " + src.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + src.string() + " config user.name tester").c_str());
    std::ofstream(src / "file.txt") << "hello";
    std::system(("git -C " + src.string() + " add file.txt").c_str());
    std::system(("git -C " + src.string() + " commit -m init > /dev/null 2>&1").c_str());
    std::system(("git -C " + src.string() + " push origin master > /dev/null 2>&1").c_str());
    REQUIRE(std::system(("git clone " + remote.string() + " " + repo.string() + " > /dev/null 2>&1").c_str()) == 0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());

    std::ofstream(src / "file.txt", std::ios::app) << "update";
    std::system(("git -C " + src.string() + " add file.txt").c_str());
    std::system(("git -C " + src.string() + " commit -m update > /dev/null 2>&1").c_str());
    std::system(("git -C " + src.string() + " push origin master > /dev/null 2>&1").c_str());

    std::vector<fs::path> repos{repo};
    std::map<fs::path, RepoInfo> infos;
    std::set<fs::path> skip;
    std::mutex mtx;
    std::atomic<bool> scanning(true);
    std::atomic<bool> running(true);
    std::string action;
    std::mutex action_mtx;

    scan_repos(repos, infos, skip, mtx, scanning, running, action, action_mtx, false,
               "origin", fs::path(), false, true, 1, 0, 0, 0, 0, 0, false, false, true,
               false, false, false, false, fs::path(), std::chrono::seconds(0), false,
               std::chrono::seconds(0), false, false, {}, false);

    REQUIRE(infos[repo].status != RS_PULL_OK);
    std::string local_hash = git::get_local_hash(repo).value_or("");
    std::string remote_hash = git::get_local_hash(src).value_or("");
    REQUIRE(local_hash != remote_hash);

    fs::remove_all(remote);
    fs::remove_all(src);
    fs::remove_all(repo);
}
