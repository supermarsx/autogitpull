#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <string>
#include <vector>

struct ParsedArgs {
    std::string root;
    bool include_private = false;
    bool show_skipped = false;
};

class ArgParser {
public:
    ParsedArgs parse(int argc, char* argv[]);
};

#endif // ARG_PARSER_H
