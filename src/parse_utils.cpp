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

std::chrono::milliseconds parse_time_ms(const std::string& value, bool& ok) {
    ok = false;
    if (value.empty())
        return std::chrono::milliseconds(0);
    std::string num = value;
    enum Unit { MS, S, M } unit = MS;
    if (num.size() > 1 && num.substr(num.size() - 2) == "ms") {
        unit = MS;
        num.resize(num.size() - 2);
    } else if (num.back() == 's') {
        unit = S;
        num.pop_back();
    } else if (num.back() == 'm') {
        unit = M;
        num.pop_back();
    } else if (!std::isdigit(static_cast<unsigned char>(num.back()))) {
        return std::chrono::milliseconds(0);
    }
    long long n = 0;
    try {
        n = std::stoll(num);
    } catch (...) {
        return std::chrono::milliseconds(0);
    }
    if (n < 0)
        return std::chrono::milliseconds(0);
    ok = true;
    switch (unit) {
    case S:
        return std::chrono::milliseconds(n * 1000);
    case M:
        return std::chrono::milliseconds(n * 60000);
    default:
        return std::chrono::milliseconds(n);
    }
}

std::chrono::milliseconds parse_time_ms(const ArgParser& parser, const std::string& flag,
                                        bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return std::chrono::milliseconds(0);
    }
    return parse_time_ms(parser.get_option(flag), ok);
}

static size_t unit_multiplier(const std::string& unit) {
    std::string u;
    for (char c : unit)
        u += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (u == "" || u == "B")
        return 1;
    if (u == "K" || u == "KB")
        return 1024ull;
    if (u == "M" || u == "MB")
        return 1024ull * 1024ull;
    if (u == "G" || u == "GB")
        return 1024ull * 1024ull * 1024ull;
    if (u == "T" || u == "TB")
        return 1024ull * 1024ull * 1024ull * 1024ull;
    if (u == "P" || u == "PB")
        return 1024ull * 1024ull * 1024ull * 1024ull * 1024ull;
    return 0;
}

size_t parse_bytes(const std::string& value, size_t min, size_t max, bool& ok) {
    ok = false;
    if (value.empty())
        return 0;
    size_t pos = value.size();
    while (pos > 0 && std::isalpha(static_cast<unsigned char>(value[pos - 1])))
        --pos;
    std::string num = value.substr(0, pos);
    std::string unit = value.substr(pos);
    if (num.empty())
        return 0;
    unsigned long long base = 0;
    try {
        base = std::stoull(num);
    } catch (...) {
        return 0;
    }
    size_t mul = unit_multiplier(unit);
    if (mul == 0)
        return 0;
    unsigned long long bytes = base * static_cast<unsigned long long>(mul);
    if (bytes < min || bytes > max)
        return 0;
    ok = true;
    return static_cast<size_t>(bytes);
}

size_t parse_bytes(const ArgParser& parser, const std::string& flag, size_t min, size_t max,
                   bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return 0;
    }
    return parse_bytes(parser.get_option(flag), min, max, ok);
}

size_t parse_bytes(const std::string& value, bool& ok) {
    return parse_bytes(value, 0, SIZE_MAX, ok);
}

size_t parse_bytes(const ArgParser& parser, const std::string& flag, bool& ok) {
    if (!parser.has_flag(flag)) {
        ok = false;
        return 0;
    }
    return parse_bytes(parser.get_option(flag), 0, SIZE_MAX, ok);
}
