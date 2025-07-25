#include "test_common.hpp"

TEST_CASE("parse_options service flags") {
    const char* argv[] = {"prog", "path",     "--install-service", "--service-config",
                          "cfg",  "--persist"};
    Options opts = parse_options(6, const_cast<char**>(argv));
    REQUIRE(opts.install_service);
    REQUIRE(opts.service_config == std::string("cfg"));
    REQUIRE(opts.persist);
    const char* argv2[] = {"prog", "path", "--uninstall-service"};
    Options opts2 = parse_options(3, const_cast<char**>(argv2));
    REQUIRE(opts2.uninstall_service);
}

TEST_CASE("parse_options attach option") {
    const char* argv[] = {"prog", "--attach", "foo"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.attach_name == std::string("foo"));
}

TEST_CASE("parse_options background option") {
    const char* argv[] = {"prog", "path", "--background", "foo"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.run_background);
    REQUIRE(opts.attach_name == std::string("foo"));
}

TEST_CASE("parse_options reattach option") {
    const char* argv[] = {"prog", "--reattach", "foo"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.reattach);
    REQUIRE(opts.attach_name == std::string("foo"));
}

TEST_CASE("parse_options runtime options") {
    const char* argv[] = {"prog", "path", "--show-runtime", "--max-runtime", "5"};
    Options opts = parse_options(5, const_cast<char**>(argv));
    REQUIRE(opts.show_runtime);
    REQUIRE(opts.runtime_limit == std::chrono::seconds(5));
}

TEST_CASE("parse_options kill-all option") {
    const char* argv[] = {"prog", "path", "--kill-all"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.kill_all);
}

TEST_CASE("parse_options rescan-new option default") {
    const char* argv[] = {"prog", "path", "--rescan-new"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.rescan_new);
    REQUIRE(opts.rescan_interval == std::chrono::minutes(5));
}

TEST_CASE("parse_options rescan-new custom interval") {
    const char* argv[] = {"prog", "path", "--rescan-new", "10"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.rescan_new);
    REQUIRE(opts.rescan_interval == std::chrono::minutes(10));
}

TEST_CASE("parse_options commit options") {
    const char* argv[] = {"prog",
                          "path",
                          "--show-commit-date",
                          "--show-commit-author",
                          "--vmem",
                          "--no-colors",
                          "--color",
                          "\033[31m",
                          "--row-order",
                          "alpha",
                          "--syslog",
                          "--syslog-facility",
                          "5"};
    Options opts = parse_options(13, const_cast<char**>(argv));
    REQUIRE(opts.show_commit_date);
    REQUIRE(opts.show_commit_author);
    REQUIRE(opts.show_vmem);
    REQUIRE(opts.no_colors);
    REQUIRE(opts.custom_color == "\033[31m");
    REQUIRE(opts.sort_mode == Options::ALPHA);
    REQUIRE(opts.use_syslog);
    REQUIRE(opts.syslog_facility == 5);
}

TEST_CASE("parse_options single repo") {
    const char* argv[] = {"prog", "path", "--single-repo"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.single_repo);
}

TEST_CASE("parse_options short flags") {
    const char* argv[] = {"prog", "path", "-x", "-m", "-w", "-X"};
    Options opts = parse_options(6, const_cast<char**>(argv));
    REQUIRE(opts.check_only);
    REQUIRE(opts.debug_memory);
    REQUIRE(opts.rescan_new);
    REQUIRE_FALSE(opts.cpu_tracker);
    REQUIRE(opts.rescan_interval == std::chrono::minutes(5));
}

TEST_CASE("parse_options dont skip timeouts") {
    const char* argv[] = {"prog", "path", "--dont-skip-timeouts"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE_FALSE(opts.skip_timeout);
}
