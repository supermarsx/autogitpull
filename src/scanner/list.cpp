#include "scanner.hpp"

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <vector>

#include "ignore_utils.hpp"

namespace fs = std::filesystem;

std::vector<fs::path> build_repo_list(const std::vector<fs::path>& roots, bool recursive,
                                      const std::vector<fs::path>& ignore, size_t max_depth) {
    std::vector<fs::path> result;
    for (const auto& root : roots) {
        if (root.empty())
            continue;
        std::error_code ec;
        fs::path canonical_root = fs::weakly_canonical(root, ec);
        if (ec) {
            ec.clear();
            canonical_root = root;
        }
        auto within_root = [&](const fs::path& p) {
            auto norm = p.lexically_normal();
            auto root_it = canonical_root.begin();
            auto p_it = norm.begin();
            for (; root_it != canonical_root.end() && p_it != norm.end(); ++root_it, ++p_it) {
                if (*root_it != *p_it)
                    return false;
            }
            return root_it == canonical_root.end();
        };
        if (recursive) {
            fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied,
                                                ec);
            fs::recursive_directory_iterator end;
            for (; it != end; it.increment(ec)) {
                if (ec) {
                    ec.clear();
                    continue;
                }
                fs::path p = it->path();
                if (fs::is_symlink(p, ec)) {
                    fs::path resolved = fs::weakly_canonical(p, ec);
                    if (ec || !within_root(resolved)) {
                        if (ec)
                            ec.clear();
                        it.disable_recursion_pending();
                        continue;
                    }
                    p = resolved;
                }
                if (max_depth > 0 && it.depth() >= static_cast<int>(max_depth)) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (!fs::is_directory(p, ec)) {
                    if (ec)
                        ec.clear();
                    continue;
                }
                if (ignore::matches(p, ignore)) {
                    it.disable_recursion_pending();
                    continue;
                }
                result.push_back(p);
            }
        } else {
            fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
            fs::directory_iterator end;
            for (; it != end; it.increment(ec)) {
                if (ec) {
                    ec.clear();
                    continue;
                }
                fs::path p = it->path();
                if (fs::is_symlink(p, ec)) {
                    fs::path resolved = fs::weakly_canonical(p, ec);
                    if (ec || !within_root(resolved)) {
                        if (ec)
                            ec.clear();
                        continue;
                    }
                    p = resolved;
                }
                if (!fs::is_directory(p, ec)) {
                    if (ec)
                        ec.clear();
                    continue;
                }
                if (ignore::matches(p, ignore))
                    continue;
                result.push_back(p);
            }
        }
    }
    return result;
}
