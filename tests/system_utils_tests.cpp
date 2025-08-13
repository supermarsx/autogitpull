#include "test_common.hpp"
#include "system_utils.hpp"
#ifndef _WIN32
#include <fcntl.h>
#include <cerrno>
#include <unistd.h>
#else
#include <windows.h>
#endif

TEST_CASE("macOS CPU affinity helpers") {
#ifdef __APPLE__
    REQUIRE_FALSE(procutil::set_cpu_affinity(0));
    REQUIRE(procutil::set_cpu_affinity(1ULL << 1));
    auto affinity = procutil::get_cpu_affinity();
    REQUIRE_FALSE(affinity.empty());
    REQUIRE(affinity.find("1") != std::string::npos);
#else
    SUCCEED();
#endif
}

TEST_CASE("UniqueFd releases descriptor") {
#ifndef _WIN32
    int raw = -1;
    {
        procutil::UniqueFd fd(::open("/dev/null", O_RDONLY));
        REQUIRE(fd);
        raw = fd.get();
    }
    errno = 0;
    REQUIRE(::close(raw) == -1);
    REQUIRE(errno == EBADF);
#else
    SUCCEED();
#endif
}

TEST_CASE("UniqueHandle releases handle") {
#ifdef _WIN32
    HANDLE raw = nullptr;
    {
        procutil::UniqueHandle h(CreateEventW(NULL, FALSE, FALSE, NULL));
        REQUIRE(h);
        raw = h.get();
    }
    REQUIRE_FALSE(CloseHandle(raw));
    REQUIRE(GetLastError() == ERROR_INVALID_HANDLE);
#else
    SUCCEED();
#endif
}

