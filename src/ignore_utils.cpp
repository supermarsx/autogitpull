#include "ignore_utils.hpp"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <regex>
#ifndef _WIN32
#include <fnmatch.h>
#endif

namespace {

void trim(std::string& s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

#ifdef _WIN32
std::string glob_to_regex(const std::string& pattern) {
    std::string rx;
    rx.reserve(pattern.size() * 2);
    rx.push_back('^');
    for (size_t i = 0; i < pattern.size(); ++i) {
        char c = pattern[i];
        if (c == '*') {
            if (i + 1 < pattern.size() && pattern[i + 1] == '*') {
                rx += ".*";
                ++i;
            } else {
                rx += "[^/]*";
            }
        } else if (c == '?') {
            rx += "[^/]";
        } else if (c == '.') {
            rx += "\\.";
        } else if (c == '\\') {
            rx += "\\\\";
        } else {
            rx.push_back(c);
        }
    }
    rx.push_back('$');
    return rx;
}
#endif

} // namespace

namespace ignore {

std::vector<std::filesystem::path> read_ignore_file(const std::filesystem::path& file) {
    std::vector<std::filesystem::path> entries;
    std::ifstream ifs(file);
    if (!ifs)
        return entries;
    std::string line;
    while (std::getline(ifs, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        entries.emplace_back(line);
    }
    return entries;
}

void write_ignore_file(const std::filesystem::path& file,
                       const std::vector<std::filesystem::path>& entries) {
    std::ofstream ofs(file, std::ios::trunc);
    for (auto e : entries) {
        std::string s = e.string();
        trim(s);
        if (s.empty())
            continue;
        ofs << s << '\n';
    }
}

bool matches(const std::filesystem::path& path,
             const std::vector<std::filesystem::path>& patterns) {
    // Use inexpensive string forms once
    const std::string full = path.generic_string();
    const std::string name = path.filename().generic_string();

    for (const auto& pat : patterns) {
        const std::string pat_str = pat.generic_string();
        const bool has_dirsep = pat_str.find('/') != std::string::npos;

        // Fast path: no glob characters -> exact match against name or full path
        const bool has_glob =
            pat_str.find('*') != std::string::npos || pat_str.find('?') != std::string::npos;
        if (!has_glob) {
            if (has_dirsep) {
                if (full == pat_str)
                    return true;
            } else {
                if (name == pat_str)
                    return true;
            }
            continue;
        }

        // Glob path: fallback to platform-appropriate matching
        if (!has_dirsep) {
#ifdef _WIN32
            std::regex re(glob_to_regex(pat_str));
            if (std::regex_match(name, re))
                return true;
#else
            if (fnmatch(pat_str.c_str(), name.c_str(), 0) == 0)
                return true;
#endif
        } else {
#ifdef _WIN32
            std::regex re(glob_to_regex(pat_str));
            if (std::regex_match(full, re))
                return true;
#else
            if (fnmatch(pat_str.c_str(), full.c_str(), 0) == 0)
                return true;
#endif
        }
    }
    return false;
}

} // namespace ignore
