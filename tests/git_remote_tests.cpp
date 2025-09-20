#include <cstdio>
#include <array>
#include <chrono>
#include "test_common.hpp"
#include "options.hpp"
#include "mutant_mode.hpp"

static std::string run_cmd(const std::string& cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return result;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
        result += buffer.data();
    pclose(pipe);
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    return result;
}

static void setup_repo(fs::path& repo, fs::path& remote, std::string& hash, std::time_t& ctime) {
    remote = fs::temp_directory_path() / "fetch_remote.git";
    repo = fs::temp_directory_path() / "fetch_local";
    FS_REMOVE_ALL(remote);
    FS_REMOVE_ALL(repo);
    REQUIRE(std::system(("git init --bare " + remote.string() + REDIR).c_str()) == 0);
    REQUIRE(std::system(("git clone " + remote.string() + " " + repo.string() + REDIR).c_str()) ==
            0);
    std::system((std::string("git -C ") + repo.string() + " config user.email you@example.com").c_str());
    std::system((std::string("git -C ") + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system((std::string("git -C ") + repo.string() + " add file.txt").c_str());
    std::system((std::string("git -C ") + repo.string() + " commit -m init" REDIR).c_str());
    REQUIRE(std::system((std::string("git -C ") + repo.string() + " push origin master" + REDIR).c_str()) == 0);
    hash = git::get_local_hash(repo).value_or("");
    std::string t = run_cmd(std::string("git -C ") + repo.string() + " log -1 --format=%ct");
    ctime = static_cast<std::time_t>(std::stoll(t));
}

TEST_CASE("get_remote_hash handles authentication options") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path repo, remote;
    std::string hash;
    std::time_t ctime;
    setup_repo(repo, remote, hash, ctime);

    bool auth_failed = false;
    auto r1 = git::get_remote_hash(repo, "origin", "master", false, &auth_failed);
    REQUIRE(r1);
    REQUIRE(*r1 == hash);
    REQUIRE_FALSE(auth_failed);

    setenv("GIT_USERNAME", "user", 1);
    setenv("GIT_PASSWORD", "pass", 1);
    auth_failed = false;
    auto r2 = git::get_remote_hash(repo, "origin", "master", true, &auth_failed);
    REQUIRE(r2);
    REQUIRE(*r2 == hash);
    REQUIRE_FALSE(auth_failed);
    unsetenv("GIT_USERNAME");
    unsetenv("GIT_PASSWORD");

    FS_REMOVE_ALL(repo);
    FS_REMOVE_ALL(remote);
}

TEST_CASE("get_remote_commit_time handles authentication options") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path repo, remote;
    std::string hash;
    std::time_t ctime;
    setup_repo(repo, remote, hash, ctime);

    bool auth_failed = false;
    REQUIRE(git::get_remote_commit_time(repo, "origin", "master", false, &auth_failed) == ctime);
    REQUIRE_FALSE(auth_failed);

    setenv("GIT_USERNAME", "user", 1);
    setenv("GIT_PASSWORD", "pass", 1);
    auth_failed = false;
    REQUIRE(git::get_remote_commit_time(repo, "origin", "master", true, &auth_failed) == ctime);
    REQUIRE_FALSE(auth_failed);
    unsetenv("GIT_USERNAME");
    unsetenv("GIT_PASSWORD");

    FS_REMOVE_ALL(repo);
    FS_REMOVE_ALL(remote);
}

TEST_CASE("remote queries fail fast on fetch error") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path repo = fs::temp_directory_path() / "fetch_fail_repo";
    fs::path bogus = fs::temp_directory_path() / "not_a_repo";
    FS_REMOVE_ALL(repo);
    FS_REMOVE_ALL(bogus);
    fs::create_directory(repo);
    fs::create_directory(bogus);
    REQUIRE(std::system(("git init " + repo.string() + REDIR).c_str()) == 0);
    std::system((std::string("git -C ") + repo.string() + " config user.email you@example.com").c_str());
    std::system((std::string("git -C ") + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system((std::string("git -C ") + repo.string() + " add file.txt").c_str());
    std::system((std::string("git -C ") + repo.string() + " commit -m init" + REDIR).c_str());
    REQUIRE(std::system((std::string("git -C ") + repo.string() + " remote add origin " + bogus.string() + REDIR)
                            .c_str()) == 0);

    bool auth_failed = false;
    std::string err;
    auto hash = git::get_remote_hash(repo, "origin", "master", false, &auth_failed, &err);
    REQUIRE_FALSE(hash);
    REQUIRE(!err.empty());
    REQUIRE_FALSE(auth_failed);

    auth_failed = false;
    auto t = git::get_remote_commit_time(repo, "origin", "master", false, &auth_failed);
    REQUIRE(t == 0);
    REQUIRE_FALSE(auth_failed);

    FS_REMOVE_ALL(repo);
    FS_REMOVE_ALL(bogus);
}

TEST_CASE("mutant_should_pull skips unchanged repos") {
    if (!have_git()) {
        WARN("git not available; skipping");
        return;
    }
    git::GitInitGuard guard;
    fs::path repo, remote;
    std::string hash;
    std::time_t ctime;
    setup_repo(repo, remote, hash, ctime);

    Options opts;
    opts.mutant_mode = true;
    opts.mutant_config = fs::temp_directory_path() / "mutant_test.cfg";
    FS_REMOVE(opts.mutant_config);
    apply_mutant_mode(opts);

    RepoInfo ri;
    ri.branch = "master";
    REQUIRE(mutant_should_pull(repo, ri, "origin", false, std::chrono::hours(1)));
    REQUIRE_FALSE(mutant_should_pull(repo, ri, "origin", false, std::chrono::hours(1)));

    FS_REMOVE_ALL(repo);
    FS_REMOVE_ALL(remote);
    FS_REMOVE(opts.mutant_config);
}
