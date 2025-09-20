#include "test_common.hpp"
#include <random>
#include <string>

TEST_CASE("rate limits throttle git pull") {
    git::GitInitGuard guard;
    if (std::system("git --version " REDIR) != 0) {
        WARN("git not available; skipping");
        return;
    }

    using namespace std::chrono;
    fs::path remote = fs::temp_directory_path() / "limit_remote.git";
    fs::path src = fs::temp_directory_path() / "limit_src";
    fs::path repo = fs::temp_directory_path() / "limit_repo";
    FS_REMOVE_ALL(remote);
    FS_REMOVE_ALL(src);
    FS_REMOVE_ALL(repo);

    REQUIRE(std::system(("git init --bare " + remote.string() + REDIR).c_str()) == 0);
    REQUIRE(std::system(("git clone " + remote.string() + " " + src.string() + REDIR).c_str()) ==
            0);
    std::system(("git -C " + src.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + src.string() + " config user.name tester").c_str());

    std::mt19937 rng(123);
    std::string payload(64 * 1024, '\0');
    for (char& c : payload)
        c = static_cast<char>(rng() % 256);
    {
        std::ofstream(src / "big1.bin") << payload;
        std::system(("git -C " + src.string() + " add big1.bin").c_str());
        std::system(("git -C " + src.string() + " commit -m init" REDIR).c_str());
        std::system(("git -C " + src.string() + " push origin master" REDIR).c_str());
    }

    REQUIRE(std::system(("git clone " + remote.string() + " " + repo.string() + REDIR).c_str()) ==
            0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());

    {
        std::ofstream(src / "big2.bin") << payload;
        std::system(("git -C " + src.string() + " add big2.bin").c_str());
        std::system(("git -C " + src.string() + " commit -m second" REDIR).c_str());
        std::system(("git -C " + src.string() + " push origin master" REDIR).c_str());
    }

    std::string log;
    bool auth_fail = false;
    std::vector<int> updates_base;
    std::function<void(int)> cb_base = [&](int pct) { updates_base.push_back(pct); };
    auto start = steady_clock::now();
    int ret = git::try_pull(repo, "origin", log, &cb_base, false, &auth_fail, 0, 0, 0, false);
    auto base_ms = duration_cast<milliseconds>(steady_clock::now() - start).count();
    REQUIRE(ret == 0);
    REQUIRE_FALSE(updates_base.empty());
    REQUIRE(updates_base.back() == 100);

    {
        std::ofstream(src / "big3.bin") << payload;
        std::system(("git -C " + src.string() + " add big3.bin").c_str());
        std::system(("git -C " + src.string() + " commit -m third" REDIR).c_str());
        std::system(("git -C " + src.string() + " push origin master" REDIR).c_str());
    }

    std::vector<int> updates_limited;
    std::function<void(int)> cb_limited = [&](int pct) { updates_limited.push_back(pct); };
    start = steady_clock::now();
    ret = git::try_pull(repo, "origin", log, &cb_limited, false, &auth_fail, 20, 20, 20, false);
    auto limited_ms = duration_cast<milliseconds>(steady_clock::now() - start).count();
    REQUIRE(ret == 0);
    REQUIRE_FALSE(updates_limited.empty());
    REQUIRE(updates_limited.back() == 100);

    REQUIRE(limited_ms - base_ms >= 5);

    FS_REMOVE_ALL(remote);
    FS_REMOVE_ALL(src);
    FS_REMOVE_ALL(repo);
}
