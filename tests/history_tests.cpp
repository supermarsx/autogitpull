#include "test_common.hpp"

TEST_CASE("history append capped at 100") {
    fs::path file = fs::temp_directory_path() / "hist_test.txt";
    fs::remove(file);
    for (int i = 0; i < 105; ++i)
        append_history(file, std::to_string(i));
    auto hist = read_history(file);
    REQUIRE(hist.size() == 100);
    REQUIRE(hist.front() == "5");
    REQUIRE(hist.back() == "104");
    fs::remove(file);
}

TEST_CASE("parse_options enable history default") {
    const char* argv[] = {"prog", "path", "--enable-history"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.enable_history);
    REQUIRE(opts.history_file == ".autogitpull.config");
}

TEST_CASE("parse_options enable history custom file") {
    const char* argv[] = {"prog", "path", "--enable-history=myhist"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.enable_history);
    REQUIRE(opts.history_file == "myhist");
}
