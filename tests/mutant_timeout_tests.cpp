#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "mutant_mode.hpp"
#include "options.hpp"
#include "test_common.hpp"

namespace fs = std::filesystem;

TEST_CASE("mutant mode adjusts pull timeout based on results") {
    Options opts;
    opts.mutant_mode = true;
    opts.mutant_config = fs::temp_directory_path() / "mutant_timeout.cfg";
    FS_REMOVE(opts.mutant_config);
    apply_mutant_mode(opts);
    REQUIRE(opts.limits.pull_timeout == std::chrono::seconds(30));

    mutant_record_result(fs::path("/tmp/repo"), RS_TIMEOUT, std::chrono::seconds(30));
    REQUIRE(opts.limits.pull_timeout == std::chrono::seconds(35));

    mutant_record_result(fs::path("/tmp/repo"), RS_PULL_OK, std::chrono::seconds(1));
    REQUIRE(opts.limits.pull_timeout == std::chrono::seconds(30));

    FS_REMOVE(opts.mutant_config);
}
