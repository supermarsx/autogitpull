#ifndef IGNORE_UTILS_HPP
#define IGNORE_UTILS_HPP
#include <filesystem>
#include <vector>

namespace ignore {
std::vector<std::filesystem::path> read_ignore_file(const std::filesystem::path& file);
void write_ignore_file(const std::filesystem::path& file,
                       const std::vector<std::filesystem::path>& entries);
} // namespace ignore

#endif // IGNORE_UTILS_HPP
