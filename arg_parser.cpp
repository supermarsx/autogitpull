#include "arg_parser.h"
#include <iostream>

ParsedArgs ArgParser::parse(int argc, char* argv[]) {
    ParsedArgs args;
    if (argc < 2) {
        return args;
    }
    args.root = argv[1];
    for (int i = 2; i < argc; ++i) {
        std::string opt = argv[i];
        if (opt == "--include-private") {
            args.include_private = true;
        } else if (opt == "--show-skipped") {
            args.show_skipped = true;
        } else {
            std::cerr << "Unknown option: " << opt << "\n";
        }
    }
    return args;
}
