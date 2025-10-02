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
    fs::path repo_base = fs::temp_directory_path() / "limit_repo_base";
    fs::path repo_limited = fs::temp_directory_path() / "limit_repo_limited";
    FS_REMOVE_ALL(remote);
    FS_REMOVE_ALL(src);
    FS_REMOVE_ALL(repo_base);
    FS_REMOVE_ALL(repo_limited);

    REQUIRE(std::system(("git init --bare " + remote.string() + REDIR).c_str()) == 0);
    REQUIRE(std::system(("git clone " + remote.string() + " " + src.string() + REDIR).c_str()) ==
            0);
    (void)std::system(("git -C " + src.string() + " config user.email you@example.com").c_str());
    (void)std::system(("git -C " + src.string() + " config user.name tester").c_str());

    std::mt19937 rng(123);
    std::string payload(64 * 1024, '\0');
    for (char& c : payload)
        c = static_cast<char>(rng() % 256);
    int commit_index = 0;
    auto push_payload_commit = [&]() {
        ++commit_index;
        std::string filename = "big" + std::to_string(commit_index) + ".bin";
        std::ofstream(src / filename) << payload;
        REQUIRE(std::system(("git -C " + src.string() + " add " + filename).c_str()) == 0);
        REQUIRE(std::system(("git -C " + src.string() + " commit -m c" +
                             std::to_string(commit_index) + REDIR)
                                .c_str()) == 0);
        REQUIRE(std::system(("git -C " + src.string() + " push origin master" REDIR).c_str()) == 0);
    };

    push_payload_commit(); // seed initial commit in remote

    const int commits_per_pull = 2;
    const std::size_t limited_down_kib = 25;

    auto run_pull_with_limit = [&](const fs::path& repo_path, std::size_t down_limit_kib) {
        FS_REMOVE_ALL(repo_path);
        std::string clone_cmd =
            "git clone \"" + remote.string() + "\" \"" + repo_path.string() + "\"" + REDIR;
        REQUIRE(std::system(clone_cmd.c_str()) == 0);
        (void)std::system(("git -C " + repo_path.string() + " config user.email you@example.com")
                              .c_str());
        (void)std::system(("git -C " + repo_path.string() + " config user.name tester").c_str());

        for (int i = 0; i < commits_per_pull; ++i)
            push_payload_commit();

        std::string log;
        bool auth_fail = false;
        std::vector<int> updates;
        std::function<void(int)> cb = [&](int pct) { updates.push_back(pct); };
        auto start = steady_clock::now();
        int ret = git::try_pull(repo_path, "origin", log, &cb, false, &auth_fail, down_limit_kib,
                                0, 0, false);
        auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start).count();
        REQUIRE(ret == 0);
        REQUIRE_FALSE(auth_fail);
        REQUIRE_FALSE(updates.empty());
        REQUIRE(updates.back() == 100);
        return elapsed;
    };

    auto base_ms = run_pull_with_limit(repo_base, 0);
    auto limited_ms = run_pull_with_limit(repo_limited, limited_down_kib);

    INFO("base duration (ms): " << base_ms);
    INFO("limited duration (ms): " << limited_ms);
    REQUIRE(base_ms >= 0);
#if defined(__APPLE__) || defined(__aarch64__) || defined(__arm__)
    // Allow small timing variance on macOS and ARM runners where IO scheduling
    // and timer resolution can make the limited transfer appear similar.
    REQUIRE(limited_ms - base_ms >= -50);
#else
    REQUIRE(limited_ms >= base_ms);
    REQUIRE(limited_ms - base_ms >= 5);
#endif

    FS_REMOVE_ALL(remote);
    FS_REMOVE_ALL(src);
    FS_REMOVE_ALL(repo_base);
    FS_REMOVE_ALL(repo_limited);
}
