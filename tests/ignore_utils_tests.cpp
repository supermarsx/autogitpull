#include "test_common.hpp"
#include "ignore_utils.hpp"

TEST_CASE("read_ignore_file trims whitespace and skips comments") {
    fs::path dir = fs::temp_directory_path() / "ign_test_parse";
    fs::create_directories(dir);
    fs::path file = dir / ".autogitpull.ignore";
    std::ofstream ofs(file);
    ofs << "  foo  \n#comment\nbar\n   \n\t#another\n\tbaz  \n";
    ofs.close();
    auto entries = ignore::read_ignore_file(file);
    std::vector<fs::path> expected{"foo", "bar", "baz"};
    REQUIRE(entries == expected);
    fs::remove_all(dir);
}

TEST_CASE("write_ignore_file skips blanks and preserves newline") {
    fs::path dir = fs::temp_directory_path() / "ign_test_write";
    fs::create_directories(dir);
    fs::path file = dir / ".autogitpull.ignore";
    std::vector<fs::path> entries{"foo", "", "bar", "  ", "baz"};
    ignore::write_ignore_file(file, entries);
    std::ifstream ifs(file, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    REQUIRE(content == "foo\nbar\nbaz\n");
    auto read = ignore::read_ignore_file(file);
    std::vector<fs::path> expected{"foo", "bar", "baz"};
    REQUIRE(read == expected);
    fs::remove_all(dir);
}
