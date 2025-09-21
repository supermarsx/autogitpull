#include "time_utils.hpp"
#include <chrono>
#include <ctime>

std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

std::string format_duration_short(std::chrono::seconds dur) {
    long long total = dur.count();
    long long s = total % 60;
    long long m = (total / 60) % 60;
    long long h = (total / 3600) % 24;
    long long d = total / 86400;
    std::string out;
    if (d > 0)
        out += std::to_string(d) + "d";
    if (h > 0 || d > 0)
        out += std::to_string(h) + "h";
    if (m > 0 || h > 0 || d > 0)
        out += std::to_string(m) + "m";
    out += std::to_string(s) + "s";
    return out;
}
