#ifndef ARG_PARSER_HPP
#define ARG_PARSER_HPP
#include <string>
#include <set>
#include <vector>

class ArgParser {
    std::set<std::string> flags_;
    std::vector<std::string> positional_;
public:
    ArgParser(int argc, char* argv[]) {
        for(int i=1;i<argc;++i) {
            std::string arg = argv[i];
            if(arg.rfind("--",0)==0) {
                flags_.insert(arg);
            } else {
                positional_.push_back(arg);
            }
        }
    }
    bool has_flag(const std::string& flag) const { return flags_.count(flag)>0; }
    const std::set<std::string>& flags() const { return flags_; }
    const std::vector<std::string>& positional() const { return positional_; }
};

#endif // ARG_PARSER_HPP
