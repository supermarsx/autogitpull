#ifndef HISTORY_UTILS_HPP
#define HISTORY_UTILS_HPP
#include <filesystem>
#include <string>
#include <vector>

std::vector<std::string> read_history(const std::filesystem::path& path);
void append_history(const std::filesystem::path& path, const std::string& entry,
                    std::size_t max_entries = 100);

#endif // HISTORY_UTILS_HPP
