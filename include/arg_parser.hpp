#ifndef ARG_PARSER_HPP
#define ARG_PARSER_HPP
#include <string>
#include <set>
#include <vector>
#include <map>

/**
 * @brief Simple command line argument parser.
 *
 * The parser recognizes long style options (e.g. `--flag` or `--opt value`).
 * A list of known flags can be provided so that unknown flags are collected
 * and reported separately. Options may also be specified using the form
 * `--opt=value`. A mapping of short options (like `-h`) to their long
 * counterparts can optionally be supplied. New options such as `--debug-memory`,
 * `--dump-state` and `--dump-large` can be added to the known flag set so they
 * are treated as valid by the parser.
 */
class ArgParser {
    std::set<std::string> flags_;                ///< Flags present on the command line
    std::map<std::string, std::string> options_; ///< Option values keyed by flag
    std::map<std::string, std::vector<std::string>>
        multi_options_;                      ///< Store all values for repeatable options
    std::vector<std::string> positional_;    ///< Positional arguments in order
    std::vector<std::string> unknown_flags_; ///< Flags not present in known_flags
    std::set<std::string> known_flags_;      ///< List of accepted flags
    std::map<char, std::string> short_map_;  ///< Mapping of short to long flags

  public:
    /**
     * @brief Parse the given command line arguments.
     *
     * @param argc Argument count from `main`.
     * @param argv Argument vector from `main`.
     * @param known_flags Optional set of flags that are considered valid. If
     *        empty, all flags are treated as known.
     * @param short_map Mapping from single character options (e.g. '-h') to
     *        their long form (e.g. '--help').
     */
    ArgParser(int argc, char* argv[], const std::set<std::string>& known_flags = {},
              const std::map<char, std::string>& short_map = {})
        : known_flags_(known_flags), short_map_(short_map) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.rfind("--", 0) == 0) {
                size_t eq = arg.find('=');
                if (eq != std::string::npos) {
                    std::string key = arg.substr(0, eq);
                    std::string val = arg.substr(eq + 1);
                    if (known_flags_.empty() || known_flags_.count(key)) {
                        flags_.insert(key);
                        options_[key] = val;
                        multi_options_[key].push_back(val);
                    } else {
                        unknown_flags_.push_back(key);
                    }
                } else if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) {
                    std::string key = arg;
                    std::string val = argv[++i];
                    if (known_flags_.empty() || known_flags_.count(key)) {
                        flags_.insert(key);
                        options_[key] = val;
                        multi_options_[key].push_back(val);
                    } else {
                        unknown_flags_.push_back(key);
                    }
                } else {
                    if (known_flags_.empty() || known_flags_.count(arg)) {
                        flags_.insert(arg);
                    } else {
                        unknown_flags_.push_back(arg);
                    }
                }
            } else if (arg.rfind('-', 0) == 0 && arg.size() >= 2 && short_map_.count(arg[1])) {
                size_t eq = arg.find('=');
                std::string before =
                    arg.substr(1, eq != std::string::npos ? eq - 1 : std::string::npos);
                std::string after = eq != std::string::npos ? arg.substr(eq + 1) : "";

                for (size_t j = 0; j < before.size();) {
                    char c = before[j];
                    if (!short_map_.count(c))
                        break;

                    std::string key = short_map_.at(c);
                    std::string val;

                    bool last = (j == before.size() - 1);
                    if (last) {
                        if (!after.empty()) {
                            val = after;
                        } else if (j + 1 < before.size() && !short_map_.count(before[j + 1])) {
                            val = before.substr(j + 1);
                            j = before.size();
                        } else if (i + 1 < argc && std::string(argv[i + 1]).rfind('-', 0) != 0) {
                            val = argv[++i];
                        }
                    } else if (!short_map_.count(before[j + 1])) {
                        val = before.substr(j + 1);
                        if (!after.empty())
                            val += after;
                        j = before.size();
                    }

                    if (!val.empty()) {
                        if (known_flags_.empty() || known_flags_.count(key)) {
                            flags_.insert(key);
                            options_[key] = val;
                            multi_options_[key].push_back(val);
                        } else {
                            unknown_flags_.push_back(key);
                        }
                        break;
                    } else {
                        if (known_flags_.empty() || known_flags_.count(key)) {
                            flags_.insert(key);
                        } else {
                            unknown_flags_.push_back(key);
                        }
                        ++j;
                    }
                }
            } else {
                positional_.push_back(arg);
            }
        }
    }

    /**
     * @brief Check whether a flag was provided on the command line.
     *
     * @param flag Flag name including the leading `--`.
     * @return `true` if the flag was present, otherwise `false`.
     */
    bool has_flag(const std::string& flag) const { return flags_.count(flag) > 0; }

    /**
     * @brief Retrieve the value associated with an option.
     *
     * If the option was not provided, an empty string is returned.
     *
     * @param opt Option name including the leading `--`.
     * @return Stored option value or empty string if missing.
     */
    std::string get_option(const std::string& opt) const {
        auto it = options_.find(opt);
        if (it != options_.end())
            return it->second;
        return "";
    }

    /**
     * @brief Retrieve all values associated with an option.
     *
     * If the option was not provided, an empty vector is returned.
     */
    std::vector<std::string> get_all_options(const std::string& opt) const {
        auto it = multi_options_.find(opt);
        if (it != multi_options_.end())
            return it->second;
        return {};
    }

    /** @return Set of all flags found during parsing. */
    const std::set<std::string>& flags() const { return flags_; }

    /** @return Map of option names to their parsed values. */
    const std::map<std::string, std::string>& options() const { return options_; }

    /** @return Ordered list of positional arguments. */
    const std::vector<std::string>& positional() const { return positional_; }

    /** @return Flags that were not part of @a known_flags. */
    const std::vector<std::string>& unknown_flags() const { return unknown_flags_; }
};

#endif // ARG_PARSER_HPP
