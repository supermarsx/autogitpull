#include "parse_utils.hpp"
#include <limits>

int parse_int(const std::string& value, int min, int max, bool& ok) {
    ok = false;
    try {
        int v = std::stoi(value);
        if (v < min || v > max)
            return 0;
        ok = true;
        return v;
    } catch (...) {
        return 0;
    }
}

int parse_int(const ArgParser& parser, const std::string& flag, int min, int max, bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return 0;
    }
    return parse_int(parser.get_option(flag), min, max, ok);
}

unsigned int parse_uint(const std::string& value, unsigned int min, unsigned int max, bool& ok) {
    ok = false;
    try {
        unsigned long v = std::stoul(value);
        if (v < min || v > max)
            return 0;
        ok = true;
        return static_cast<unsigned int>(v);
    } catch (...) {
        return 0;
    }
}

unsigned int parse_uint(const ArgParser& parser, const std::string& flag, unsigned int min,
                        unsigned int max, bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return 0;
    }
    return parse_uint(parser.get_option(flag), min, max, ok);
}

size_t parse_size_t(const std::string& value, size_t min, size_t max, bool& ok) {
    ok = false;
    try {
        unsigned long long v = std::stoull(value);
        if (v < min || v > max)
            return 0;
        ok = true;
        return static_cast<size_t>(v);
    } catch (...) {
        return 0;
    }
}

size_t parse_size_t(const ArgParser& parser, const std::string& flag, size_t min, size_t max,
                    bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return 0;
    }
    return parse_size_t(parser.get_option(flag), min, max, ok);
}

unsigned long long parse_ull(const std::string& value, unsigned long long min,
                             unsigned long long max, bool& ok) {
    ok = false;
    try {
        unsigned long long v = std::stoull(value, nullptr, 0);
        if (v < min || v > max)
            return 0;
        ok = true;
        return v;
    } catch (...) {
        return 0;
    }
}

unsigned long long parse_ull(const ArgParser& parser, const std::string& flag,
                             unsigned long long min, unsigned long long max, bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return 0;
    }
    return parse_ull(parser.get_option(flag), min, max, ok);
}
