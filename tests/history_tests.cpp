#include "test_common.hpp"

struct DirGuard {
    std::filesystem::path old;
    explicit DirGuard(const std::filesystem::path& p) : old(std::filesystem::current_path()) {
        std::filesystem::current_path(p);
    }
    ~DirGuard() { std::filesystem::current_path(old); }
};

TEST_CASE("history append capped at 100") {
    fs::path file = fs::temp_directory_path() / "hist_test.txt";
    fs::remove(file);
    for (int i = 0; i < 105; ++i)
        append_history(file, std::to_string(i));
    auto hist = read_history(file);
    REQUIRE(hist.size() == 100);
    REQUIRE(hist.front() == "5");
    REQUIRE(hist.back() == "104");
    fs::remove(file);
}

TEST_CASE("parse_options enable history default") {
    const char* argv[] = {"prog", "path", "--enable-history"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.enable_history);
    REQUIRE(opts.history_file == ".autogitpull.config");
}

TEST_CASE("parse_options enable history custom file") {
    const char* argv[] = {"prog", "path", "--enable-history=myhist"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.enable_history);
    REQUIRE(opts.history_file == "myhist");
}

TEST_CASE("enable history auto-detect root") {
    fs::path root_dir = fs::temp_directory_path() / "hist_root";
    fs::path cwd_dir = fs::temp_directory_path() / "hist_cwd";
    fs::path exe_dir = fs::temp_directory_path() / "hist_exe";
    fs::create_directories(root_dir);
    fs::create_directories(cwd_dir);
    fs::create_directories(exe_dir);
    std::ofstream(root_dir / ".autogitpull.config") << "line\n";
    DirGuard guard(cwd_dir);
    std::string exe = (exe_dir / "prog").string();
    std::string root_s = root_dir.string();
    char* argv[] = {const_cast<char*>(exe.c_str()), const_cast<char*>(root_s.c_str()),
                    const_cast<char*>("--enable-history")};
    Options opts = parse_options(3, argv);
    REQUIRE(opts.history_file == (root_dir / ".autogitpull.config").string());
    auto hist = read_history(opts.history_file);
    REQUIRE(hist.size() == 1);
    REQUIRE(hist[0] == "line");
    fs::remove_all(root_dir);
    fs::remove_all(cwd_dir);
    fs::remove_all(exe_dir);
}

TEST_CASE("enable history auto-detect cwd fallback") {
    fs::path root_dir = fs::temp_directory_path() / "hist_root2";
    fs::path cwd_dir = fs::temp_directory_path() / "hist_cwd2";
    fs::path exe_dir = fs::temp_directory_path() / "hist_exe2";
    fs::create_directories(root_dir);
    fs::create_directories(cwd_dir);
    fs::create_directories(exe_dir);
    std::ofstream(cwd_dir / ".autogitpull.config") << "line2\n";
    DirGuard guard(cwd_dir);
    std::string exe = (exe_dir / "prog").string();
    std::string root_s = root_dir.string();
    char* argv[] = {const_cast<char*>(exe.c_str()), const_cast<char*>(root_s.c_str()),
                    const_cast<char*>("--enable-history")};
    Options opts = parse_options(3, argv);
    REQUIRE(opts.history_file == (cwd_dir / ".autogitpull.config").string());
    fs::remove_all(root_dir);
    fs::remove_all(cwd_dir);
    fs::remove_all(exe_dir);
}

TEST_CASE("enable history auto-detect exe fallback") {
    fs::path root_dir = fs::temp_directory_path() / "hist_root3";
    fs::path cwd_dir = fs::temp_directory_path() / "hist_cwd3";
    fs::path exe_dir = fs::temp_directory_path() / "hist_exe3";
    fs::create_directories(root_dir);
    fs::create_directories(cwd_dir);
    fs::create_directories(exe_dir);
    std::ofstream(exe_dir / ".autogitpull.config") << "line3\n";
    DirGuard guard(cwd_dir);
    std::string exe = (exe_dir / "prog").string();
    std::string root_s = root_dir.string();
    char* argv[] = {const_cast<char*>(exe.c_str()), const_cast<char*>(root_s.c_str()),
                    const_cast<char*>("--enable-history")};
    Options opts = parse_options(3, argv);
    REQUIRE(opts.history_file == (exe_dir / ".autogitpull.config").string());
    fs::remove_all(root_dir);
    fs::remove_all(cwd_dir);
    fs::remove_all(exe_dir);
}
