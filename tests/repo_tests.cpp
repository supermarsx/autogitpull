#include "test_common.hpp"
#include <chrono>
#include <vector>

TEST_CASE("get_local_hash surfaces error for missing repo") {
    git::GitInitGuard guard;
    fs::path repo = fs::temp_directory_path() / "nonexistent_repo";
    FS_REMOVE_ALL(repo);
    std::string err;
    auto hash = git::get_local_hash(repo, &err);
    REQUIRE_FALSE(hash);
    REQUIRE(!err.empty());
}

TEST_CASE("get_remote_hash surfaces error for missing remote") {
    git::GitInitGuard guard;
    fs::path repo = fs::temp_directory_path() / "missing_remote_repo";
    FS_REMOVE_ALL(repo);
    fs::create_directory(repo);
    REQUIRE(std::system(("git init " + repo.string() + REDIR).c_str()) == 0);
    std::system((std::string("git -C ") + repo.string() + " config user.email you@example.com").c_str());
    std::system((std::string("git -C ") + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system((std::string("git -C ") + repo.string() + " add file.txt").c_str());
    std::system((std::string("git -C ") + repo.string() + " commit -m init" + REDIR).c_str());
    std::string err;
    auto hash = git::get_remote_hash(repo, "origin", "master", false, nullptr, &err);
    REQUIRE_FALSE(hash);
    REQUIRE(!err.empty());
    FS_REMOVE_ALL(repo);
}

TEST_CASE("Git utils local repo") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path repo = fs::temp_directory_path() / "git_utils_test_repo";
    FS_REMOVE_ALL(repo);
    fs::create_directory(repo);
    REQUIRE(git::is_git_repo(repo) == false);

    std::string cmd = "git init " + repo.string() + REDIR;
    REQUIRE(std::system(cmd.c_str()) == 0);
    std::system((std::string("git -C ") + repo.string() + " config user.email you@example.com").c_str());
    std::system((std::string("git -C ") + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system((std::string("git -C ") + repo.string() + " add file.txt").c_str());
    std::system((std::string("git -C ") + repo.string() + " commit -m init" + REDIR).c_str());

    REQUIRE(git::is_git_repo(repo));
    std::string branch = git::get_current_branch(repo).value_or("");
    REQUIRE(!branch.empty());
    std::string hash = git::get_local_hash(repo).value_or("");
    REQUIRE(hash.size() == 40);
    FS_REMOVE_ALL(repo);
}

TEST_CASE("Git utils GitHub url detection") {
    REQUIRE(git::is_github_url("https://github.com/user/repo.git"));
    REQUIRE_FALSE(git::is_github_url("https://gitlab.com/user/repo.git"));
}

TEST_CASE("clone_repo clones a public repository") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path dest = fs::temp_directory_path() / "clone_repo_test";
    FS_REMOVE_ALL(dest);
    bool auth_failed = false;
    bool ok = git::clone_repo(dest, "https://github.com/octocat/Hello-World.git", nullptr, false,
                              &auth_failed);
    if (!ok) {
        const git_error* e = git_error_last();
        WARN("clone_repo failed: " << (e && e->message ? e->message : "unknown"));
        return;
    }
    REQUIRE_FALSE(auth_failed);
    REQUIRE(git::is_git_repo(dest));
    FS_REMOVE_ALL(dest);
}

TEST_CASE("clone_repo reports progress and respects limits") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path dest = fs::temp_directory_path() / "clone_repo_progress";
    FS_REMOVE_ALL(dest);
    std::vector<int> updates;
    std::function<void(int)> cb = [&](int pct) { updates.push_back(pct); };
    auto start = std::chrono::steady_clock::now();
    bool auth_failed = false;
    bool ok = git::clone_repo(dest, "https://github.com/octocat/Hello-World.git", &cb, false,
                              &auth_failed, 1, 1, 1);
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (!ok) {
        const git_error* e = git_error_last();
        WARN("clone_repo failed: " << (e && e->message ? e->message : "unknown"));
        return;
    }
    REQUIRE_FALSE(auth_failed);
    REQUIRE_FALSE(updates.empty());
    REQUIRE(updates.back() == 100);
    REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= 500);
    FS_REMOVE_ALL(dest);
}

TEST_CASE("RepoInfo defaults") {
    RepoInfo ri;
    REQUIRE(ri.status == RS_PENDING);
    REQUIRE(ri.progress == 0);
    REQUIRE_FALSE(ri.auth_failed);
}

TEST_CASE("build_repo_list ignores directories") {
    fs::path root = fs::temp_directory_path() / "ignore_test";
    FS_REMOVE_ALL(root);
    fs::create_directories(root / "a");
    fs::create_directories(root / "b");
    fs::create_directories(root / "c");

    std::vector<fs::path> ignore{root / "b", root / "c"};
    std::vector<fs::path> repos = build_repo_list({root}, false, ignore, 0);
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "b") == repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "c") == repos.end());

    FS_REMOVE_ALL(root);
}

TEST_CASE("build_repo_list skips files") {
    fs::path root = fs::temp_directory_path() / "file_scan_test";
    FS_REMOVE_ALL(root);
    fs::create_directories(root / "repo");
    std::ofstream(root / "file.txt") << "ignore";

    std::vector<fs::path> ignore;
    std::vector<fs::path> repos = build_repo_list({root}, false, ignore, 0);
    REQUIRE(std::find(repos.begin(), repos.end(), root / "repo") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "file.txt") == repos.end());

    FS_REMOVE_ALL(root);
}

TEST_CASE("build_repo_list ignores symlinks outside root") {
    fs::path tmp = fs::temp_directory_path();
    fs::path root = tmp / "symlink_scan_root";
    fs::path outside = tmp / "symlink_outside";
    FS_REMOVE_ALL(root);
    FS_REMOVE_ALL(outside);
    fs::create_directories(root / "repo");
    fs::create_directories(outside / "repo");
    fs::path link = root / "outside_link";
    fs::create_directory_symlink(outside, link);

    std::vector<fs::path> ignore;
    std::vector<fs::path> repos = build_repo_list({root}, false, ignore, 0);
    REQUIRE(std::find(repos.begin(), repos.end(), root / "repo") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), link) == repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), outside) == repos.end());

    FS_REMOVE_ALL(root);
    FS_REMOVE_ALL(outside);
}

TEST_CASE("recursive iterator finds nested repo") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path root = fs::temp_directory_path() / "recursive_test";
    FS_REMOVE_ALL(root);
    fs::create_directories(root / "sub/inner/nested");
    fs::path repo = root / "sub/inner/nested";
    REQUIRE(std::system(("git init " + repo.string() + REDIR).c_str()) == 0);
    std::system((std::string("git -C ") + repo.string() + " config user.email you@example.com").c_str());
    std::system((std::string("git -C ") + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system((std::string("git -C ") + repo.string() + " add file.txt").c_str());
    std::system((std::string("git -C ") + repo.string() + " commit -m init" + REDIR).c_str());

    std::vector<fs::path> flat;
    for (const auto& e : fs::directory_iterator(root))
        flat.push_back(e.path());
    REQUIRE(std::find(flat.begin(), flat.end(), repo) == flat.end());

    std::vector<fs::path> rec;
    for (const auto& e : fs::recursive_directory_iterator(root))
        if (e.is_directory())
            rec.push_back(e.path());
    REQUIRE(std::find(rec.begin(), rec.end(), repo) != rec.end());

    FS_REMOVE_ALL(root);
}

TEST_CASE("build_repo_list respects max depth") {
    fs::path root = fs::temp_directory_path() / "depth_test";
    FS_REMOVE_ALL(root);
    fs::create_directories(root / "a/b/c");

    std::vector<fs::path> ignore;
    std::vector<fs::path> repos = build_repo_list({root}, true, ignore, 2);
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a/b") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a/b/c") == repos.end());

    FS_REMOVE_ALL(root);
}

TEST_CASE("build_repo_list scans multiple roots") {
    fs::path r1 = fs::temp_directory_path() / "multi_root1";
    fs::path r2 = fs::temp_directory_path() / "multi_root2";
    FS_REMOVE_ALL(r1);
    FS_REMOVE_ALL(r2);
    fs::create_directories(r1 / "a");
    fs::create_directories(r2 / "b");

    std::vector<fs::path> ignore;
    std::vector<fs::path> repos = build_repo_list({r1, r2}, false, ignore, 0);
    REQUIRE(std::find(repos.begin(), repos.end(), r1 / "a") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), r2 / "b") != repos.end());

    FS_REMOVE_ALL(r1);
    FS_REMOVE_ALL(r2);
}

TEST_CASE("build_repo_list handles large ignore set") {
    fs::path root = fs::temp_directory_path() / "large_ignore_test";
    FS_REMOVE_ALL(root);
    fs::create_directory(root);
    constexpr int N = 1000;
    std::vector<fs::path> dirs;
    dirs.reserve(N);
    for (int i = 0; i < N; ++i) {
        fs::path d = root / ("dir" + std::to_string(i));
        fs::create_directory(d);
        dirs.push_back(d);
    }
    std::vector<fs::path> ignore(dirs.begin(), dirs.end());
    ignore.pop_back();
    auto start = std::chrono::steady_clock::now();
    std::vector<fs::path> repos = build_repo_list({root}, false, ignore, 0);
    auto dur = std::chrono::steady_clock::now() - start;
    REQUIRE(std::find(repos.begin(), repos.end(), dirs.back()) != repos.end());
    REQUIRE(dur < std::chrono::seconds(2));
    FS_REMOVE_ALL(root);
}

TEST_CASE("scan_repos respects concurrency limit") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    std::vector<fs::path> repos;
    for (int i = 0; i < 3; ++i) {
        fs::path repo = fs::temp_directory_path() / ("threads_repo_" + std::to_string(i));
        FS_REMOVE_ALL(repo);
        fs::create_directory(repo);
        REQUIRE(std::system(("git init " + repo.string() + REDIR).c_str()) == 0);
        std::system((std::string("git -C ") + repo.string() + " config user.email you@example.com").c_str());
        std::system((std::string("git -C ") + repo.string() + " config user.name tester").c_str());
        std::ofstream(repo / "file.txt") << "hello";
        std::system((std::string("git -C ") + repo.string() + " add file.txt").c_str());
        std::system((std::string("git -C ") + repo.string() + " commit -m init" + REDIR).c_str());
        repos.push_back(repo);
    }

    std::map<fs::path, RepoInfo> infos;
    for (const auto& p : repos)
        infos[p] = RepoInfo{p, RS_PENDING, "", "", "", "", "", 0, "", 0, false, false};
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
        scan_repos(repos, infos, skip, mtx, scanning, running, act, act_mtx, false, "origin",
                   fs::path(), true, true, concurrency, 0, 0, 0, 0, 0, true, false, false, false,
                   true, true, false, fs::path(), std::chrono::seconds(0), false,
                   std::chrono::seconds(0), false, false, {}, false);
    });
    while (scanning) {
        max_seen = std::max(max_seen, read_thread_count());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    t.join();

    REQUIRE(max_seen - baseline <= concurrency);

    for (const auto& p : repos)
        FS_REMOVE_ALL(p);
}

TEST_CASE("try_pull handles dirty repos") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path remote = fs::temp_directory_path() / "pull_remote.git";
    fs::path src = fs::temp_directory_path() / "pull_src";
    fs::path repo = fs::temp_directory_path() / "pull_work";
    FS_REMOVE_ALL(remote);
    FS_REMOVE_ALL(src);
    FS_REMOVE_ALL(repo);
    REQUIRE(std::system(("git init --bare " + remote.string() + REDIR).c_str()) == 0);
    REQUIRE(std::system(("git clone " + remote.string() + " " + src.string() + REDIR).c_str()) ==
            0);
    std::system((std::string("git -C ") + src.string() + " config user.email you@example.com").c_str());
    std::system((std::string("git -C ") + src.string() + " config user.name tester").c_str());
    std::ofstream(src / "file.txt") << "hello";
    std::system((std::string("git -C ") + src.string() + " add file.txt").c_str());
    std::system((std::string("git -C ") + src.string() + " commit -m init" + REDIR).c_str());
    std::system((std::string("git -C ") + src.string() + " push origin master" + REDIR).c_str());
    REQUIRE(std::system(("git clone " + remote.string() + " " + repo.string() + REDIR).c_str()) ==
            0);
    std::system((std::string("git -C ") + repo.string() + " config user.email you@example.com").c_str());
    std::system((std::string("git -C ") + repo.string() + " config user.name tester").c_str());
    std::ofstream(src / "file.txt", std::ios::app) << "update";
    std::system((std::string("git -C ") + src.string() + " add file.txt").c_str());
    std::system((std::string("git -C ") + src.string() + " commit -m update" + REDIR).c_str());
    std::system((std::string("git -C ") + src.string() + " push origin master" + REDIR).c_str());
    std::ofstream(repo / "file.txt", std::ios::app) << "local";

    std::string log;
    bool auth_fail = false;
    int ret = git::try_pull(repo, "origin", log, nullptr, false, &auth_fail, 0, 0, 0, false);
    REQUIRE(ret == 3);
    REQUIRE(fs::exists(repo / "file.txt"));
    std::string after = git::get_local_hash(repo).value_or("");
    REQUIRE(after != git::get_local_hash(src).value_or(""));

    ret = git::try_pull(repo, "origin", log, nullptr, false, &auth_fail, 0, 0, 0, true);
    REQUIRE(ret == 0);
    {
        std::ifstream ifs(repo / "file.txt");
        std::string contents;
        std::getline(ifs, contents);
        REQUIRE(contents == "helloupdate");
    }
    REQUIRE(git::get_local_hash(repo).value_or("") == git::get_local_hash(src).value_or(""));

    FS_REMOVE_ALL(remote);
    FS_REMOVE_ALL(src);
    FS_REMOVE_ALL(repo);
}

TEST_CASE("scan_repos resets statuses to pending") {
    std::map<fs::path, RepoInfo> infos;
    fs::path p = fs::temp_directory_path() / "pending_status_repo";
    RepoInfo ri;
    ri.path = p;
    ri.status = RS_PULL_OK;
    ri.message = "done";
    infos[p] = ri;
    std::set<fs::path> skip;
    std::mutex mtx;
    std::atomic<bool> scanning(true);
    std::atomic<bool> running(true);
    std::string act;
    std::mutex act_mtx;

    scan_repos({}, infos, skip, mtx, scanning, running, act, act_mtx, false, "origin", fs::path(),
               true, true, 1, 0, 0, 0, 0, 0, true, false, false, false, true, true, false,
               fs::path(), std::chrono::seconds(0), false, std::chrono::seconds(0), false, false,
               {}, false);

    REQUIRE(infos[p].status == RS_PENDING);
    REQUIRE(infos[p].progress == 0);
}
