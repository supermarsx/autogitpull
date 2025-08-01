#include "ignore_utils.hpp"
#include <fstream>
#include <algorithm>

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
        if (!line.empty())
            entries.push_back(line);
    }
    return entries;
}

void write_ignore_file(const std::filesystem::path& file,
                       const std::vector<std::filesystem::path>& entries) {
    std::ofstream ofs(file, std::ios::trunc);
    for (const auto& e : entries)
        ofs << e.string() << '\n';
}

} // namespace ignore
