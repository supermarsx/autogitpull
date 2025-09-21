#include "test_common.hpp"
#include "scanner.hpp"

namespace fs = std::filesystem;

TEST_CASE("run_post_pull_hook executes script") {
    fs::path marker = fs::temp_directory_path() / "hook_marker";
#if defined(_WIN32)
    fs::path hook = fs::temp_directory_path() / "post_hook.bat";
    {
        std::ofstream ofs(hook);
        ofs << "@echo off\n";
        ofs << "echo ran > \"" << marker.string() << "\"\n";
    }
#else
    fs::path hook = fs::temp_directory_path() / "post_hook.sh";
    {
        std::ofstream ofs(hook);
        ofs << "#!/bin/sh\n";
        ofs << "echo ran > \"" << marker.string() << "\"\n";
    }
    fs::permissions(hook, fs::perms::owner_exec | fs::perms::owner_write | fs::perms::owner_read);
#endif
    run_post_pull_hook(hook);
    REQUIRE(fs::exists(marker));
    FS_REMOVE(hook);
    FS_REMOVE(marker);
}
