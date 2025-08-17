#ifndef IGNORE_UTILS_HPP
#define IGNORE_UTILS_HPP
#include <filesystem>
#include <vector>

namespace ignore {

/**
 * Read a list of ignore entries from a file.
 *
 * Each non-empty line in the file is treated as a distinct path entry. If a
 * line ends with a carriage return (`\r`), it is stripped before storing the
 * path. Paths are not normalised, so they may be relative or absolute and are
 * stored exactly as provided.
 *
 * Missing or unreadable files result in an empty list of entries.
 */
std::vector<std::filesystem::path> read_ignore_file(const std::filesystem::path& file);

/**
 * Write ignore entries to a file.
 *
 * Each path is written on its own line using its string representation. The
 * target file is truncated if it exists or created if missing.
 */
void write_ignore_file(const std::filesystem::path& file,
                       const std::vector<std::filesystem::path>& entries);
} // namespace ignore

#endif // IGNORE_UTILS_HPP
