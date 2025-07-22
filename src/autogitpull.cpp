#include <filesystem>
#include <iostream>

#include "options.hpp"
#include "ui_loop.hpp"
#include "git_utils.hpp"
#include "lock_utils.hpp"
#include "version.hpp"

namespace fs = std::filesystem;

#ifndef AUTOGITPULL_NO_MAIN
int main(int argc, char* argv[]) {
    git::GitInitGuard git_guard;
    try {
        Options opts = parse_options(argc, argv);
        if (opts.show_help) {
            print_help(argv[0]);
            return 0;
        }
        if (opts.print_version) {
            std::cout << AUTOGITPULL_VERSION << "\n";
            return 0;
        }
        if (opts.remove_lock) {
            if (!opts.root.empty()) {
                fs::path lock = opts.root / ".autogitpull.lock";
                procutil::release_lock_file(lock);
            }
            return 0;
        }
        return run_event_loop(opts);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
#endif // AUTOGITPULL_NO_MAIN
