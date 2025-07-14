#ifndef ARG_PARSER_HPP
#define ARG_PARSER_HPP
#include <string>
#include <set>
#include <vector>
#include <map>

class ArgParser {
    std::set<std::string> flags_;
    std::map<std::string, std::string> options_;
    std::vector<std::string> positional_;
public:
    ArgParser(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.rfind("--", 0) == 0) {
                size_t eq = arg.find('=');
                if (eq != std::string::npos) {
                    std::string key = arg.substr(0, eq);
                    std::string val = arg.substr(eq + 1);
                    flags_.insert(key);
                    options_[key] = val;
                } else if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) {
                    std::string key = arg;
                    std::string val = argv[++i];
                    flags_.insert(key);
                    options_[key] = val;
                } else {
                    flags_.insert(arg);
                }
            } else {
                positional_.push_back(arg);
            }
        }
    }

    bool has_flag(const std::string& flag) const { return flags_.count(flag) > 0; }
    std::string get_option(const std::string& opt) const {
        auto it = options_.find(opt);
        if (it != options_.end()) return it->second;
        return "";
    }
    const std::set<std::string>& flags() const { return flags_; }
    const std::map<std::string, std::string>& options() const { return options_; }
    const std::vector<std::string>& positional() const { return positional_; }
};

#endif // ARG_PARSER_HPP
