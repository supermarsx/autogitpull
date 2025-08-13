#include "test_common.hpp"
#include "cli_commands.hpp"

TEST_CASE("handle_hard_reset requires confirmation") {
    Options opts;
    opts.hard_reset = true;
    auto rc = cli::handle_hard_reset(opts);
    REQUIRE(rc.has_value());
    REQUIRE(*rc == 1);
}

TEST_CASE("handle_status_queries list instances") {
    Options opts;
    opts.list_instances = true;
    auto rc = cli::handle_status_queries(opts);
    REQUIRE(rc.has_value());
    REQUIRE(*rc == 0);
}

TEST_CASE("handle_hard_reset executes when confirmed") {
    Options opts;
    opts.hard_reset = true;
    opts.confirm_reset = true;
    auto rc = cli::handle_hard_reset(opts);
    REQUIRE(rc.has_value());
    REQUIRE(*rc == 0);
}
