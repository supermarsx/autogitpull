#ifndef DEBUG_UTILS_HPP
#define DEBUG_UTILS_HPP
#include <cstddef>
#include <string>
#include <sstream>
#include <filesystem>
#include <map>
#include <set>
#include <vector>
#include "logger.hpp"
#include "repo.hpp"

namespace debug_utils {

template <typename C> size_t approx_bytes(const C& c) {
    return c.size() * sizeof(typename C::value_type);
}

template <typename C> void log_container_size(const std::string& name, const C& c) {
    log_debug(name + " count=" + std::to_string(c.size()) + " bytes~" +
              std::to_string(approx_bytes(c)));
}

void dump_repo_infos(const std::map<std::filesystem::path, RepoInfo>& infos);

template <typename C> void dump_container(const std::string& name, const C& c) {
    std::ostringstream oss;
    oss << name << "(" << c.size() << ")";
    for (const auto& v : c)
        oss << "\n" << v;
    log_debug(oss.str());
}

} // namespace debug_utils

#endif // DEBUG_UTILS_HPP
