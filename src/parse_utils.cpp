#include "parse_utils.hpp"
#include <limits>
#include <chrono>
#include <cctype>

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

double parse_double(const std::string& value, double min, double max, bool& ok) {
    ok = false;
    try {
        size_t idx = 0;
        double v = std::stod(value, &idx);
        if (idx != value.size())
            return 0.0;
        auto pos = value.find('.');
        if (pos != std::string::npos && value.size() - pos - 1 > 1)
            return 0.0;
        if (v < min || v > max)
            return 0.0;
        ok = true;
        return v;
    } catch (...) {
        return 0.0;
    }
}

double parse_double(const ArgParser& parser, const std::string& flag, double min, double max,
                    bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return 0.0;
    }
    return parse_double(parser.get_option(flag), min, max, ok);
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
