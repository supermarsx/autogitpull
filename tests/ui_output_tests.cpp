#include <sstream>
#include <iostream>
#include "test_common.hpp"
#include "tui.hpp"
#include "ui_loop.hpp"
// Forward declaration of draw_cli from ui_loop.cpp
void draw_cli(const std::vector<fs::path>& all_repos,
              const std::map<fs::path, RepoInfo>& repo_infos, int seconds_left,
              bool scanning, const std::string& action, bool show_skipped,
              bool show_notgit, int runtime_sec, bool show_repo_count,
              bool session_dates_only, bool censor_names, char censor_char);

TEST_CASE("draw_cli shows active repo count") {
    std::vector<fs::path> repos = {"/a", "/b", "/c", "/d"};
    std::map<fs::path, RepoInfo> infos;
    infos[repos[0]] = RepoInfo{repos[0], RS_UP_TO_DATE, "", "", "", "", "", "", 0, false};
    infos[repos[1]] = RepoInfo{repos[1], RS_SKIPPED, "", "", "", "", "", "", 0, false};
    infos[repos[2]] = RepoInfo{repos[2], RS_NOT_GIT, "", "", "", "", "", "", 0, false};
    infos[repos[3]] = RepoInfo{repos[3], RS_PENDING, "", "", "", "", "", "", 0, false};
    std::ostringstream oss;
    auto* old = std::cout.rdbuf(oss.rdbuf());
    draw_cli(repos, infos, 10, false, "Idle", true, false, -1, true, false, false, '*');
    std::cout.rdbuf(old);
    std::string out = oss.str();
    REQUIRE(out.rfind("Repos: 2/4\n", 0) == 0);
}

TEST_CASE("render_header reports repo count") {
    std::vector<fs::path> repos = {"/a", "/b", "/c", "/d"};
    std::map<fs::path, RepoInfo> infos;
    infos[repos[0]] = RepoInfo{repos[0], RS_UP_TO_DATE, "", "", "", "", "", "", 0, false};
    infos[repos[1]] = RepoInfo{repos[1], RS_SKIPPED, "", "", "", "", "", "", 0, false};
    infos[repos[2]] = RepoInfo{repos[2], RS_NOT_GIT, "", "", "", "", "", "", 0, false};
    infos[repos[3]] = RepoInfo{repos[3], RS_PENDING, "", "", "", "", "", "", 0, false};
    TuiColors colors = make_tui_colors(true, "");
    std::string out = render_header(repos, infos, 5, 1, false, "Idle", false, true, "", -1, false, colors);
    REQUIRE(out.find("Repos: 2/4\n") != std::string::npos);
}

TEST_CASE("render_repo_entry censors names") {
    fs::path repo = "/foo/bar";
    RepoInfo ri{repo, RS_UP_TO_DATE, "", "", "", "", "", "", 0, false};
    TuiColors colors = make_tui_colors(true, "");
    std::string out = render_repo_entry(repo, ri, true, true, false, false, false, true, '#', colors);
    REQUIRE(out.find("bar") == std::string::npos);
    REQUIRE(out.find("###") != std::string::npos);
}
