#include <cstdlib>
#include <catch2/catch_test_macros.hpp>
#include "macos_daemon.hpp"

#ifdef __APPLE__
TEST_CASE("launchctl command generation", "[macos]") {
    setenv("AUTOGITPULL_UNIT_DIR", "/tmp/agents", 1);
    REQUIRE(procutil::plist_path("svc") == std::string("/tmp/agents/svc.plist"));
    REQUIRE(procutil::launchctl_command("load", "svc") ==
            std::string("launchctl load /tmp/agents/svc.plist" REDIR));
}
#endif
