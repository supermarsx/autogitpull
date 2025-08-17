#ifndef HISTORY_UTILS_HPP
#define HISTORY_UTILS_HPP
#include <filesystem>
#include <string>
#include <vector>

/**
 * @brief Read history entries from a newline-delimited text file.
 *
 * Each line in the file represents a single UTF-8 encoded entry.
 *
 * @param path Filesystem location of the history file.
 * @return Vector of entries in the order they appear in the file.
 */
std::vector<std::string> read_history(const std::filesystem::path& path);

/**
 * @brief Append an entry to the history file, trimming old entries.
 *
 * The file is assumed to be newline-delimited UTF-8 text. The function appends
 * @a entry as a new line and retains at most @a max_entries newest lines.
 *
 * @param path Filesystem location of the history file.
 * @param entry Entry to append as a single line.
 * @param max_entries Maximum number of entries to keep in the file.
 */
void append_history(const std::filesystem::path& path, const std::string& entry,
                    std::size_t max_entries = 100);

#endif // HISTORY_UTILS_HPP
