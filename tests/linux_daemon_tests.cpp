#include "test_common.hpp"
#include "linux_daemon.hpp"

TEST_CASE("service unit names are not shell expanded") {
#ifndef _WIN32
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "autogitpull_mock_systemctl";
    fs::create_directories(dir);
    fs::path script = dir / "systemctl";
    fs::path argsfile = dir / "args";
    {
        std::ofstream out(script);
        out << "#!/bin/sh\n";
        out << "printf '%s\\n' \"$@\" > '" << argsfile.string() << "'\n";
    }
    fs::permissions(script, fs::perms::owner_read | fs::perms::owner_write |
                                fs::perms::owner_exec);
    std::string old_path = std::getenv("PATH") ? std::getenv("PATH") : "";
    setenv("PATH", (dir.string() + ":" + old_path).c_str(), 1);
    fs::path marker = dir / "marker";
    std::string unit = "foo; touch " + marker.string();
    REQUIRE(procutil::start_service_unit(unit));
    setenv("PATH", old_path.c_str(), 1);

    std::ifstream in(argsfile);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line))
        lines.push_back(line);
    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0] == "start");
    REQUIRE(lines[1] == unit);
    REQUIRE(!fs::exists(marker));
    fs::remove_all(dir);
#else
    SUCCEED("Windows test skipped");
#endif
}

