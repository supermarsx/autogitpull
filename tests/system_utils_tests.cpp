#include "test_common.hpp"
#include "system_utils.hpp"

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

