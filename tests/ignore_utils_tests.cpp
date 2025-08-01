#include "test_common.hpp"
#include "ignore_utils.hpp"

TEST_CASE("ignore file read write") {
    fs::path dir = fs::temp_directory_path() / "ign_test";
    fs::create_directories(dir);
    fs::path file = dir / ".autogitpull.ignore";
    std::vector<fs::path> entries{dir / "a", dir / "b"};
    ignore::write_ignore_file(file, entries);
    auto read = ignore::read_ignore_file(file);
    REQUIRE(read == entries);
    fs::remove_all(dir);
}
