#include "test_common.hpp"

TEST_CASE("list_installed_services detects units") {
#ifndef _WIN32
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "autogitpull_unit_test";
    fs::create_directories(dir);
    std::ofstream(dir / "foo.service") << "[Service]\nExecStart=/usr/bin/autogitpull\n";
    std::ofstream(dir / "bar.service") << "[Service]\nExecStart=/bin/other\n";
    setenv("AUTOGITPULL_UNIT_DIR", dir.string().c_str(), 1);
    auto svcs = procutil::list_installed_services();
    unsetenv("AUTOGITPULL_UNIT_DIR");
    FS_REMOVE_ALL(dir);
    REQUIRE(svcs.size() == 1);
    REQUIRE(svcs[0].first == "foo");
    std::ostringstream oss;
    for (const auto& [name, st] : svcs)
        oss << name << " " << (st.running ? "running" : "stopped") << "\n";
    REQUIRE(oss.str() == "foo stopped\n");
#else
    SUCCEED("Windows test skipped");
#endif
}
