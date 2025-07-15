#include <catch2/catch_test_macros.hpp>
#include "arg_parser.hpp"
#include "git_utils.hpp"
#include "repo.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace fs = std::filesystem;

TEST_CASE("ArgParser basic parsing") {
    const char* argv[] = {
        "prog", "--foo", "--opt", "42", "pos", "--unknown" };
    ArgParser parser(6, const_cast<char**>(argv), {"--foo","--bar","--opt"});
    REQUIRE(parser.has_flag("--foo"));
    REQUIRE(parser.get_option("--opt") == "42");
    REQUIRE(parser.positional().size() == 1);
    REQUIRE(parser.positional()[0] == "pos");
    REQUIRE(parser.unknown_flags().size() == 1);
    REQUIRE(parser.unknown_flags()[0] == "--unknown");
}

TEST_CASE("ArgParser option with equals") {
    const char* argv[] = { "prog", "--opt=val" };
    ArgParser parser(2, const_cast<char**>(argv), {"--opt"});
    REQUIRE(parser.has_flag("--opt"));
    REQUIRE(parser.get_option("--opt") == "val");
}

TEST_CASE("ArgParser unknown flag detection") {
    const char* argv[] = { "prog", "--foo" };
    ArgParser parser(2, const_cast<char**>(argv), {"--bar"});
    REQUIRE(parser.has_flag("--foo") == false);
    REQUIRE(parser.unknown_flags().size() == 1);
    REQUIRE(parser.unknown_flags()[0] == "--foo");
}

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


