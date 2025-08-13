#include <catch2/catch_test_macros.hpp>
#include "windows_service.hpp"
#include "lock_utils.hpp"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <thread>
#include <chrono>

TEST_CASE("windows named pipe attach", "[windows]") {
    std::string name = "pipe-test";
    int srv_fd = winservice::create_status_socket(name);
    REQUIRE(srv_fd >= 0);
    HANDLE srv_h = reinterpret_cast<HANDLE>(_get_osfhandle(srv_fd));
    std::thread server([&]() {
        ConnectNamedPipe(srv_h, nullptr);
        const char msg[] = "hello";
        DWORD written = 0;
        WriteFile(srv_h, msg, sizeof(msg) - 1, &written, nullptr);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    int cli_fd = winservice::connect_status_socket(name);
    REQUIRE(cli_fd >= 0);
    HANDLE cli_h = reinterpret_cast<HANDLE>(_get_osfhandle(cli_fd));
    char buf[16] = {};
    DWORD read = 0;
    ReadFile(cli_h, buf, sizeof(buf), &read, nullptr);
    REQUIRE(std::string(buf, read) == "hello");
    server.join();
    winservice::remove_status_socket(name, srv_fd);
    _close(cli_fd);
}

TEST_CASE("find_running_instances detects pipe", "[windows]") {
    std::string name = "detect-test";
    int srv_fd = winservice::create_status_socket(name);
    REQUIRE(srv_fd >= 0);
    HANDLE srv_h = reinterpret_cast<HANDLE>(_get_osfhandle(srv_fd));
    std::thread server([&]() { ConnectNamedPipe(srv_h, nullptr); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto instances = procutil::find_running_instances();
    bool found = false;
    for (const auto& inst : instances) {
        if (inst.first == name) {
            found = true;
            REQUIRE(inst.second == static_cast<unsigned long>(GetCurrentProcessId()));
        }
    }
    REQUIRE(found);
    server.join();
    winservice::remove_status_socket(name, srv_fd);
}
#endif
