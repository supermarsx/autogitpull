#include "parse_utils.hpp"
#include <limits>
#include <chrono>
#include <cctype>
#include <climits>

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

std::chrono::seconds parse_duration(const std::string& value, bool& ok) {
    ok = false;
    if (value.empty())
        return std::chrono::seconds(0);
    char unit = value.back();
    std::string num = value;
    if (unit == 's' || unit == 'm' || unit == 'h' || unit == 'd' || unit == 'w' || unit == 'M') {
        num.pop_back();
    } else if (std::isdigit(static_cast<unsigned char>(unit))) {
        unit = 's';
    } else {
        return std::chrono::seconds(0);
    }
    long long n = 0;
    try {
        n = std::stoll(num);
    } catch (...) {
        return std::chrono::seconds(0);
    }
    if (n < 0)
        return std::chrono::seconds(0);
    ok = true;
    switch (unit) {
    case 'm':
        return std::chrono::minutes(n);
    case 'h':
        return std::chrono::hours(n);
    case 'd':
        return std::chrono::hours(24 * n);
    case 'w':
        return std::chrono::hours(24 * 7 * n);
    case 'M':
        return std::chrono::hours(24 * 30 * n);
    default:
        return std::chrono::seconds(n);
    }
}

std::chrono::seconds parse_duration(const ArgParser& parser, const std::string& flag, bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return std::chrono::seconds(0);
    }
    return parse_duration(parser.get_option(flag), ok);
}

size_t parse_bytes(const std::string& value, size_t min, size_t max, bool& ok) {
    ok = false;
    if (value.empty())
        return 0;
    std::string val = value;
    std::transform(val.begin(), val.end(), val.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    unsigned long long mult = 1;
    auto ends_with = [&](const std::string& suf) {
        return val.size() >= suf.size() &&
               val.compare(val.size() - suf.size(), suf.size(), suf) == 0;
    };
    if (ends_with("kb")) {
        mult = 1024ull;
        val.erase(val.size() - 2);
    } else if (ends_with("mb")) {
        mult = 1024ull * 1024;
        val.erase(val.size() - 2);
    } else if (ends_with("gb")) {
        mult = 1024ull * 1024 * 1024;
        val.erase(val.size() - 2);
    } else if (ends_with("tb")) {
        mult = 1024ull * 1024 * 1024 * 1024;
        val.erase(val.size() - 2);
    } else if (ends_with("pb")) {
        mult = 1024ull * 1024 * 1024 * 1024 * 1024;
        val.erase(val.size() - 2);
    } else if (!val.empty() && val.back() == 'k') {
        mult = 1024ull;
        val.pop_back();
    } else if (!val.empty() && val.back() == 'm') {
        mult = 1024ull * 1024;
        val.pop_back();
    } else if (!val.empty() && val.back() == 'g') {
        mult = 1024ull * 1024 * 1024;
        val.pop_back();
    } else if (!val.empty() && val.back() == 't') {
        mult = 1024ull * 1024 * 1024 * 1024;
        val.pop_back();
    } else if (!val.empty() && val.back() == 'p') {
        mult = 1024ull * 1024 * 1024 * 1024 * 1024;
        val.pop_back();
    } else if (!val.empty() && val.back() == 'b') {
        val.pop_back();
    }
    unsigned long long base = 0;
    try {
        base = std::stoull(val);
    } catch (...) {
        return 0;
    }
    if (base > ULLONG_MAX / mult)
        return 0;
    unsigned long long total = base * mult;
    if (total < min || total > max)
        return 0;
    ok = true;
    return static_cast<size_t>(total);
}

size_t parse_bytes(const ArgParser& parser, const std::string& flag, size_t min, size_t max,
                   bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return 0;
    }
    return parse_bytes(parser.get_option(flag), min, max, ok);
}
