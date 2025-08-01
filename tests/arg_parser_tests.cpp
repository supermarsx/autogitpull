#include "test_common.hpp"
#include <climits>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

TEST_CASE("ArgParser basic parsing") {
    const char* argv[] = {"prog", "--foo", "--opt", "42", "pos", "--unknown"};
    ArgParser parser(6, const_cast<char**>(argv), {"--foo", "--bar", "--opt"});
    REQUIRE(parser.has_flag("--foo"));
    REQUIRE(parser.get_option("--opt") == "42");
    REQUIRE(parser.positional().size() == 1);
    REQUIRE(parser.positional()[0] == "pos");
    REQUIRE(parser.unknown_flags().size() == 1);
    REQUIRE(parser.unknown_flags()[0] == "--unknown");
}

TEST_CASE("ArgParser option with equals") {
    const char* argv[] = {"prog", "--opt=val"};
    ArgParser parser(2, const_cast<char**>(argv), {"--opt"});
    REQUIRE(parser.has_flag("--opt"));
    REQUIRE(parser.get_option("--opt") == "val");
}

TEST_CASE("ArgParser short options") {
    const char* argv[] = {"prog", "-h", "-o42"};
    ArgParser parser(3, const_cast<char**>(argv), {"--help", "--opt"},
                     {{'h', "--help"}, {'o', "--opt"}});
    REQUIRE(parser.has_flag("--help"));
    REQUIRE(parser.get_option("--opt") == std::string("42"));
}

TEST_CASE("ArgParser stacked short flags") {
    const char* argv[] = {"prog", "-abc"};
    ArgParser parser(2, const_cast<char**>(argv), {"--flag-a", "--flag-b", "--flag-c"},
                     {{'a', "--flag-a"}, {'b', "--flag-b"}, {'c', "--flag-c"}});
    REQUIRE(parser.has_flag("--flag-a"));
    REQUIRE(parser.has_flag("--flag-b"));
    REQUIRE(parser.has_flag("--flag-c"));
}

TEST_CASE("ArgParser unknown flag detection") {
    const char* argv[] = {"prog", "--foo"};
    ArgParser parser(2, const_cast<char**>(argv), {"--bar"});
    REQUIRE_FALSE(parser.has_flag("--foo"));
    REQUIRE(parser.unknown_flags().size() == 1);
    REQUIRE(parser.unknown_flags()[0] == "--foo");
}

TEST_CASE("ArgParser unknown short flag") {
    const char* argv[] = {"prog", "-x"};
    ArgParser parser(2, const_cast<char**>(argv), {"--bar"}, {{'a', "--bar"}});
    REQUIRE(parser.positional().empty());
    REQUIRE(parser.unknown_flags().size() == 1);
    REQUIRE(parser.unknown_flags()[0] == "-x");
}

TEST_CASE("ArgParser log level flags") {
    const char* argv[] = {"prog", "--log-level", "DEBUG", "--verbose"};
    ArgParser parser(4, const_cast<char**>(argv), {"--log-level", "--verbose"});
    REQUIRE(parser.has_flag("--log-level"));
    REQUIRE(parser.get_option("--log-level") == "DEBUG");
    REQUIRE(parser.has_flag("--verbose"));
}

TEST_CASE("ArgParser log file option") {
    const char* argv[] = {"prog", "--log-file", "my.log", "path"};
    ArgParser parser(4, const_cast<char**>(argv), {"--log-file"});
    REQUIRE(parser.has_flag("--log-file"));
    REQUIRE(parser.get_option("--log-file") == "my.log");
    REQUIRE(parser.positional().size() == 1);
    REQUIRE(parser.positional()[0] == "path");
}

TEST_CASE("ArgParser resource flags") {
    const char* argv[] = {"prog", "--max-threads", "4",   "--cpu-percent", "50", "--cpu-cores",
                          "2",    "--mem-limit",   "100", "path"};
    ArgParser parser(10, const_cast<char**>(argv),
                     {"--max-threads", "--cpu-percent", "--cpu-cores", "--mem-limit"});
    REQUIRE(parser.get_option("--max-threads") == std::string("4"));
    REQUIRE(parser.get_option("--cpu-percent") == std::string("50"));
    REQUIRE(parser.get_option("--cpu-cores") == std::string("2"));
    REQUIRE(parser.get_option("--mem-limit") == std::string("100"));
    REQUIRE(parser.positional().size() == 1);
    REQUIRE(parser.positional()[0] == std::string("path"));
}

TEST_CASE("ArgParser threads flags") {
    const char* argv[] = {"prog", "--threads", "8", "--single-thread"};
    ArgParser parser(4, const_cast<char**>(argv), {"--threads", "--single-thread"});
    REQUIRE(parser.has_flag("--threads"));
    REQUIRE(parser.get_option("--threads") == std::string("8"));
    REQUIRE(parser.has_flag("--single-thread"));
}

TEST_CASE("ArgParser network limits") {
    const char* argv[] = {"prog", "--download-limit", "100", "--upload-limit", "50"};
    ArgParser parser(5, const_cast<char**>(argv), {"--download-limit", "--upload-limit"});
    REQUIRE(parser.get_option("--download-limit") == std::string("100"));
    REQUIRE(parser.get_option("--upload-limit") == std::string("50"));
}

TEST_CASE("ArgParser disk limit") {
    const char* argv[] = {"prog", "--disk-limit", "250"};
    ArgParser parser(3, const_cast<char**>(argv), {"--disk-limit"});
    REQUIRE(parser.get_option("--disk-limit") == std::string("250"));
}

TEST_CASE("ArgParser debug flags") {
    const char* argv[] = {"prog", "--debug-memory", "--dump-state", "--dump-large", "5"};
    ArgParser parser(5, const_cast<char**>(argv),
                     {"--debug-memory", "--dump-state", "--dump-large"});
    REQUIRE(parser.has_flag("--debug-memory"));
    REQUIRE(parser.has_flag("--dump-state"));
    REQUIRE(parser.get_option("--dump-large") == std::string("5"));
}

TEST_CASE("ArgParser recursive flag") {
    const char* argv[] = {"prog", "--recursive"};
    ArgParser parser(2, const_cast<char**>(argv), {"--recursive"});
    REQUIRE(parser.has_flag("--recursive"));
}

TEST_CASE("ArgParser daemon flags") {
    const char* argv[] = {"prog", "--install-daemon", "--daemon-config", "cfg", "--persist"};
    ArgParser parser(5, const_cast<char**>(argv),
                     {"--install-daemon", "--uninstall-daemon", "--daemon-config", "--persist"});
    REQUIRE(parser.has_flag("--install-daemon"));
    REQUIRE(parser.get_option("--daemon-config") == std::string("cfg"));
    REQUIRE(parser.has_flag("--persist"));
    const char* argv2[] = {"prog", "--uninstall-daemon"};
    ArgParser parser2(2, const_cast<char**>(argv2), {"--install-daemon", "--uninstall-daemon"});
    REQUIRE(parser2.has_flag("--uninstall-daemon"));
}

TEST_CASE("ArgParser persist with value") {
    const char* argv[] = {"prog", "--persist=myrun"};
    ArgParser parser(2, const_cast<char**>(argv), {"--persist"});
    REQUIRE(parser.has_flag("--persist"));
    REQUIRE(parser.get_option("--persist") == std::string("myrun"));
}

TEST_CASE("ArgParser new short flags") {
    const char* argv[] = {"prog", "-x", "-m", "-w", "-X"};
    ArgParser parser(5, const_cast<char**>(argv),
                     {"--check-only", "--debug-memory", "--rescan-new", "--no-cpu-tracker"},
                     {{'x', "--check-only"},
                      {'m', "--debug-memory"},
                      {'w', "--rescan-new"},
                      {'X', "--no-cpu-tracker"}});
    REQUIRE(parser.has_flag("--check-only"));
    REQUIRE(parser.has_flag("--debug-memory"));
    REQUIRE(parser.has_flag("--rescan-new"));
    REQUIRE(parser.has_flag("--no-cpu-tracker"));
}

TEST_CASE("parse_int helper valid") {
    const char* argv[] = {"prog", "--num", "5"};
    ArgParser parser(3, const_cast<char**>(argv), {"--num"});
    bool ok = false;
    int v = parse_int(parser, "--num", 0, 10, ok);
    REQUIRE(ok);
    REQUIRE(v == 5);
}

TEST_CASE("parse_int helper invalid") {
    const char* argv[] = {"prog", "--num", "bad"};
    ArgParser parser(3, const_cast<char**>(argv), {"--num"});
    bool ok = false;
    parse_int(parser, "--num", 0, 10, ok);
    REQUIRE_FALSE(ok);
}

TEST_CASE("parse_double helper valid") {
    const char* argv[] = {"prog", "--num", "5.5"};
    ArgParser parser(3, const_cast<char**>(argv), {"--num"});
    bool ok = false;
    double v = parse_double(parser, "--num", 0.0, 10.0, ok);
    REQUIRE(ok);
    REQUIRE(v == Approx(5.5));
}

TEST_CASE("parse_double helper invalid") {
    const char* argv[] = {"prog", "--num", "bad"};
    ArgParser parser(3, const_cast<char**>(argv), {"--num"});
    bool ok = false;
    parse_double(parser, "--num", 0.0, 10.0, ok);
    REQUIRE_FALSE(ok);
}

TEST_CASE("parse_size_t helper range") {
    const char* argv[] = {"prog", "--num", "100"};
    ArgParser parser(3, const_cast<char**>(argv), {"--num"});
    bool ok = false;
    parse_size_t(parser, "--num", 0, 50, ok);
    REQUIRE_FALSE(ok);
}

TEST_CASE("parse_bytes units") {
    bool ok = false;
    REQUIRE(parse_bytes("1KB", 0, SIZE_MAX, ok) == 1024);
    REQUIRE(ok);
    REQUIRE(parse_bytes("2MB", 0, SIZE_MAX, ok) == 2 * 1024 * 1024);
    REQUIRE(ok);
    REQUIRE(parse_bytes("3G", 0, SIZE_MAX, ok) == 3ull * 1024 * 1024 * 1024);
    REQUIRE(ok);
}

TEST_CASE("parse_time_ms units") {
    bool ok = false;
    REQUIRE(parse_time_ms("250ms", ok) == std::chrono::milliseconds(250));
    REQUIRE(ok);
    REQUIRE(parse_time_ms("2s", ok) == std::chrono::milliseconds(2000));
    REQUIRE(ok);
    REQUIRE(parse_time_ms("1m", ok) == std::chrono::milliseconds(60000));
    REQUIRE(ok);
}

TEST_CASE("parse_duration units") {
    bool ok = false;
    REQUIRE(parse_duration("5s", ok) == std::chrono::seconds(5));
    REQUIRE(ok);
    REQUIRE(parse_duration("2m", ok) == std::chrono::minutes(2));
    REQUIRE(ok);
    REQUIRE(parse_duration("1h", ok) == std::chrono::hours(1));
    REQUIRE(ok);
}
