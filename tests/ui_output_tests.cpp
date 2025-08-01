#include <sstream>
#include <iostream>
#include "test_common.hpp"
#include "ui_loop.hpp"
// Forward declaration of draw_cli from ui_loop.cpp
void draw_cli(const std::vector<fs::path>& all_repos,
              const std::map<fs::path, RepoInfo>& repo_infos, int seconds_left,
              bool scanning, const std::string& action, bool show_skipped,
              int runtime_sec, bool show_repo_count, bool session_dates_only,
              bool censor_names, char censor_char);

TEST_CASE("draw_cli shows active repo count") {
    std::vector<fs::path> repos = {"/a", "/b", "/c"};
    std::map<fs::path, RepoInfo> infos;
    infos[repos[0]] = RepoInfo{repos[0], RS_UP_TO_DATE, "", "", "", "", "", "", 0, false};
    infos[repos[1]] = RepoInfo{repos[1], RS_SKIPPED, "", "", "", "", "", "", 0, false};
    infos[repos[2]] = RepoInfo{repos[2], RS_PENDING, "", "", "", "", "", "", 0, false};
    std::ostringstream oss;
    auto* old = std::cout.rdbuf(oss.rdbuf());
    draw_cli(repos, infos, 10, false, "Idle", true, -1, true, false, false, '*');
    std::cout.rdbuf(old);
    std::string out = oss.str();
    REQUIRE(out.rfind("Repos: 2/3\n", 0) == 0);
}
