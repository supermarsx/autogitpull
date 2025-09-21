#include "history_utils.hpp"
#include <fstream>

std::vector<std::string> read_history(const std::filesystem::path& path) {
    std::vector<std::string> lines;
    std::ifstream ifs(path);
    if (!ifs)
        return lines;
    std::string line;
    while (std::getline(ifs, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

void append_history(const std::filesystem::path& path, const std::string& entry,
                    std::size_t max_entries) {
    auto lines = read_history(path);
    lines.push_back(entry);
    if (lines.size() > max_entries)
        lines.erase(lines.begin(), lines.end() - max_entries);
    std::ofstream ofs(path, std::ios::trunc);
    for (const auto& l : lines)
        ofs << l << '\n';
}
