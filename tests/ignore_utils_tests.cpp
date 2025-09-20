#include "test_common.hpp"
#include "ignore_utils.hpp"

TEST_CASE("read_ignore_file trims whitespace and skips comments") {
    fs::path dir = fs::temp_directory_path() / "ign_test_parse";
    fs::create_directories(dir);
    fs::path file = dir / ".autogitpull.ignore";
    std::ofstream ofs(file);
    ofs << R"(  foo  
#comment
bar
   
	#another
	baz  
)";
    ofs.close();
    auto entries = ignore::read_ignore_file(file);
    std::vector<fs::path> expected{"foo", "bar", "baz"};
    REQUIRE(entries == expected);
    FS_REMOVE_ALL(dir);
}

TEST_CASE("write_ignore_file skips blanks and preserves newline") {
    fs::path dir = fs::temp_directory_path() / "ign_test_write";
    fs::create_directories(dir);
    fs::path file = dir / ".autogitpull.ignore";
    std::vector<fs::path> entries{"foo", "", "bar", "  ", "baz"};
    ignore::write_ignore_file(file, entries);
    std::ifstream ifs(file, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    std::string normalized;
    normalized.reserve(content.size());
    for (char ch : content) {
        if (ch != '\r')
            normalized.push_back(ch);
    }
    REQUIRE(normalized == "foo\nbar\nbaz\n");
    ifs.close();
    auto read = ignore::read_ignore_file(file);
    std::vector<fs::path> expected{"foo", "bar", "baz"};
    REQUIRE(read == expected);
    FS_REMOVE_ALL(dir);
}

TEST_CASE("ignore pattern matching supports wildcards") {
    std::vector<fs::path> patterns{"**/build/*", "*.tmp"};
    REQUIRE(ignore::matches(fs::path("foo/build/output.o"), patterns));
    REQUIRE(ignore::matches(fs::path("dir/file.tmp"), patterns));
    REQUIRE_FALSE(ignore::matches(fs::path("src/main.cpp"), patterns));
}
