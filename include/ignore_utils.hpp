#ifndef IGNORE_UTILS_HPP
#define IGNORE_UTILS_HPP
#include <filesystem>
#include <vector>

namespace ignore {

/**
 * Read a list of ignore entries from a file.
 *
 * Each non-empty, non-comment line in the file is trimmed of leading and
 * trailing whitespace and treated as a distinct path entry. Lines beginning
 * with '#' are considered comments and skipped. If a line ends with a carriage
 * return (`\r`), it is stripped before processing. Paths are not normalised, so
 * they may be relative or absolute and are stored exactly as provided.
 *
 * Missing or unreadable files result in an empty list of entries.
 */
std::vector<std::filesystem::path> read_ignore_file(const std::filesystem::path& file);

/**
 * Write ignore entries to a file.
 *
 * Blank or whitespace-only entries are skipped. Each path is written on its
 * own line using its string representation and the file is always terminated
 * with a trailing newline. The target file is truncated if it exists or
 * created if missing.
 */
void write_ignore_file(const std::filesystem::path& file,
                       const std::vector<std::filesystem::path>& entries);
} // namespace ignore

#endif // IGNORE_UTILS_HPP
