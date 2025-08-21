#include "ignore_utils.hpp"
#include <fstream>
#include <algorithm>
#include <cctype>

namespace {

void trim(std::string& s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

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

} // namespace ignore
