#include "test_common.hpp"
#include "scanner.hpp"

namespace fs = std::filesystem;

TEST_CASE("run_post_pull_hook executes script") {
    fs::path hook = fs::temp_directory_path() / "post_hook.sh";
    fs::path marker = fs::temp_directory_path() / "hook_marker";
    {
        std::ofstream ofs(hook);
        ofs << "#!/bin/sh\n";
        ofs << "echo ran > \"" << marker.string() << "\"\n";
    }
    fs::permissions(hook, fs::perms::owner_exec | fs::perms::owner_write |
                               fs::perms::owner_read);
    run_post_pull_hook(hook);
    REQUIRE(fs::exists(marker));
    fs::remove(hook);
    fs::remove(marker);
}

