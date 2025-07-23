#include "test_common.hpp"

TEST_CASE("Git utils local repo") {
    git::GitInitGuard guard;
    fs::path repo = fs::temp_directory_path() / "git_utils_test_repo";
    fs::remove_all(repo);
    fs::create_directory(repo);
    REQUIRE(git::is_git_repo(repo) == false);

    std::string cmd = "git init " + repo.string() + " > /dev/null 2>&1";
    REQUIRE(std::system(cmd.c_str()) == 0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system(("git -C " + repo.string() + " add file.txt").c_str());
    std::system(("git -C " + repo.string() + " commit -m init > /dev/null 2>&1").c_str());

    REQUIRE(git::is_git_repo(repo));
    std::string branch = git::get_current_branch(repo);
    REQUIRE(!branch.empty());
    std::string hash = git::get_local_hash(repo);
    REQUIRE(hash.size() == 40);
    fs::remove_all(repo);
}

TEST_CASE("Git utils GitHub url detection") {
    REQUIRE(git::is_github_url("https://github.com/user/repo.git"));
    REQUIRE_FALSE(git::is_github_url("https://gitlab.com/user/repo.git"));
}

TEST_CASE("RepoInfo defaults") {
    RepoInfo ri;
    REQUIRE(ri.status == RS_PENDING);
    REQUIRE(ri.progress == 0);
    REQUIRE_FALSE(ri.auth_failed);
}

TEST_CASE("build_repo_list ignores directories") {
    fs::path root = fs::temp_directory_path() / "ignore_test";
    fs::remove_all(root);
    fs::create_directories(root / "a");
    fs::create_directories(root / "b");
    fs::create_directories(root / "c");

    std::vector<fs::path> ignore{root / "b", root / "c"};
    std::vector<fs::path> repos = build_repo_list(root, false, ignore, 0);
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "b") == repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "c") == repos.end());

    fs::remove_all(root);
}

TEST_CASE("recursive iterator finds nested repo") {
    git::GitInitGuard guard;
    fs::path root = fs::temp_directory_path() / "recursive_test";
    fs::remove_all(root);
    fs::create_directories(root / "sub/inner/nested");
    fs::path repo = root / "sub/inner/nested";
    REQUIRE(std::system(("git init " + repo.string() + " > /dev/null 2>&1").c_str()) == 0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system(("git -C " + repo.string() + " add file.txt").c_str());
    std::system(("git -C " + repo.string() + " commit -m init > /dev/null 2>&1").c_str());

    std::vector<fs::path> flat;
    for (const auto& e : fs::directory_iterator(root))
        flat.push_back(e.path());
    REQUIRE(std::find(flat.begin(), flat.end(), repo) == flat.end());

    std::vector<fs::path> rec;
    for (const auto& e : fs::recursive_directory_iterator(root))
        if (e.is_directory())
            rec.push_back(e.path());
    REQUIRE(std::find(rec.begin(), rec.end(), repo) != rec.end());

    fs::remove_all(root);
}

TEST_CASE("build_repo_list respects max depth") {
    fs::path root = fs::temp_directory_path() / "depth_test";
    fs::remove_all(root);
    fs::create_directories(root / "a/b/c");

    std::vector<fs::path> repos = build_repo_list(root, true, {}, 2);
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a/b") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a/b/c") == repos.end());

    fs::remove_all(root);
}

TEST_CASE("scan_repos respects concurrency limit") {
    git::GitInitGuard guard;
    std::vector<fs::path> repos;
    for (int i = 0; i < 3; ++i) {
        fs::path repo = fs::temp_directory_path() / ("threads_repo_" + std::to_string(i));
        fs::remove_all(repo);
        fs::create_directory(repo);
        REQUIRE(std::system(("git init " + repo.string() + " > /dev/null 2>&1").c_str()) == 0);
        std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
        std::system(("git -C " + repo.string() + " config user.name tester").c_str());
        std::ofstream(repo / "file.txt") << "hello";
        std::system(("git -C " + repo.string() + " add file.txt").c_str());
        std::system(("git -C " + repo.string() + " commit -m init > /dev/null 2>&1").c_str());
        repos.push_back(repo);
    }

    std::map<fs::path, RepoInfo> infos;
    for (const auto& p : repos)
        infos[p] = RepoInfo{p, RS_PENDING, "", "", "", "", "", "", 0, false};
    std::set<fs::path> skip;
    std::mutex mtx;
    std::atomic<bool> scanning(true);
    std::atomic<bool> running(true);
    std::string act;
    std::mutex act_mtx;

    const std::size_t concurrency = 2;
    std::size_t baseline = read_thread_count();
    std::size_t max_seen = baseline;

    std::thread t([&]() {
        scan_repos(repos, infos, skip, mtx, scanning, running, act, act_mtx, false, fs::path(),
                   true, true, concurrency, 0, 0, 0, 0, 0, true, false);
    });
    while (scanning) {
        max_seen = std::max(max_seen, read_thread_count());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    t.join();

    REQUIRE(max_seen - baseline <= concurrency);

    for (const auto& p : repos)
        fs::remove_all(p);
}

TEST_CASE("try_pull handles dirty repos") {
    git::GitInitGuard guard;
    fs::path remote = fs::temp_directory_path() / "pull_remote.git";
    fs::path src = fs::temp_directory_path() / "pull_src";
    fs::path repo = fs::temp_directory_path() / "pull_work";
    fs::remove_all(remote);
    fs::remove_all(src);
    fs::remove_all(repo);
    REQUIRE(std::system(("git init --bare " + remote.string() + " > /dev/null 2>&1").c_str()) == 0);
    REQUIRE(std::system(("git clone " + remote.string() + " " + src.string() + " > /dev/null 2>&1")
                            .c_str()) == 0);
    std::system(("git -C " + src.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + src.string() + " config user.name tester").c_str());
    std::ofstream(src / "file.txt") << "hello";
    std::system(("git -C " + src.string() + " add file.txt").c_str());
    std::system(("git -C " + src.string() + " commit -m init > /dev/null 2>&1").c_str());
    std::system(("git -C " + src.string() + " push origin master > /dev/null 2>&1").c_str());
    REQUIRE(std::system(("git clone " + remote.string() + " " + repo.string() + " > /dev/null 2>&1")
                            .c_str()) == 0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());
    std::ofstream(src / "file.txt", std::ios::app) << "update";
    std::system(("git -C " + src.string() + " add file.txt").c_str());
    std::system(("git -C " + src.string() + " commit -m update > /dev/null 2>&1").c_str());
    std::system(("git -C " + src.string() + " push origin master > /dev/null 2>&1").c_str());
    std::ofstream(repo / "file.txt", std::ios::app) << "local";

    std::string log;
    bool auth_fail = false;
    int ret = git::try_pull(repo, log, nullptr, false, &auth_fail, 0, 0, 0, false);
    REQUIRE(ret == 3);
    REQUIRE(fs::exists(repo / "file.txt"));
    std::string after = git::get_local_hash(repo);
    REQUIRE(after != git::get_local_hash(src));

    ret = git::try_pull(repo, log, nullptr, false, &auth_fail, 0, 0, 0, true);
    REQUIRE(ret == 0);
    {
        std::ifstream ifs(repo / "file.txt");
        std::string contents;
        std::getline(ifs, contents);
        REQUIRE(contents == "helloupdate");
    }
    REQUIRE(git::get_local_hash(repo) == git::get_local_hash(src));

    fs::remove_all(remote);
    fs::remove_all(src);
    fs::remove_all(repo);
}
