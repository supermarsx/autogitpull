#ifndef DEBUG_UTILS_HPP
#define DEBUG_UTILS_HPP
#include <cstddef>
#include <string>
#include <sstream>
#include <filesystem>
#include <map>
#include <set>
#include <vector>
#include <type_traits>
#include "logger.hpp"
#include "repo.hpp"

namespace debug_utils {

template <typename C> size_t approx_bytes(const C& c) {
    return c.size() * sizeof(typename C::value_type);
}

template <typename C> auto get_capacity(const C& c) -> decltype(c.capacity()) {
    return c.capacity();
}

inline std::size_t get_capacity(...) { return 0; }

template <typename C> void log_container_size(const std::string& name, const C& c) {
    log_debug(name + " count=" + std::to_string(c.size()) + " bytes~" +
              std::to_string(approx_bytes(c)));
}

template <typename C>
void log_container_delta(const std::string& name, const C& c, std::size_t& last) {
    long long delta = static_cast<long long>(c.size()) - static_cast<long long>(last);
    std::size_t cap = get_capacity(c);
    std::string msg = name + " size=" + std::to_string(c.size());
    if (cap)
        msg += " cap=" + std::to_string(cap);
    msg += " delta=" + std::to_string(delta);
    log_debug(msg);
    last = c.size();
}

/**
 * @brief Log change in resident memory usage.
 *
 * Computes the difference between @p current and @p last memory values,
 * reporting both the current usage and delta in megabytes. The arithmetic is
 * constant time, but the required string formatting and logging calls can be
 * expensive if executed in tight loops.
 *
 * @param current Current memory usage in megabytes.
 * @param last    Previous memory value; updated to @p current after logging.
 */
void log_memory_delta_mb(std::size_t current, std::size_t& last);

/**
 * @brief Emit diagnostic information for tracked repositories.
 *
 * Iterates over @p infos and writes up to @p max_items entries to the debug
 * log. Large maps or high @p max_items values increase string concatenation
 * and logging overhead, so choose the limit carefully when debugging large
 * sets.
 *
 * @param infos     Map of repository paths to status information.
 * @param max_items Maximum number of entries to output.
 */
void dump_repo_infos(const std::map<std::filesystem::path, RepoInfo>& infos,
                     std::size_t max_items = 50);

template <typename C>
void dump_container(const std::string& name, const C& c, std::size_t max_items = 50) {
    std::ostringstream oss;
    oss << name << "(" << c.size() << ")";
    std::size_t count = 0;
    for (const auto& v : c) {
        if (count++ >= max_items) {
            oss << "\n...";
            break;
        }
        oss << "\n" << v;
    }
    log_debug(oss.str());
}

} // namespace debug_utils
#define LOG_SIZE(name, container)                                                                  \
    do {                                                                                           \
        static std::size_t _last = 0;                                                              \
        ::debug_utils::log_container_delta(name, container, _last);                                \
    } while (0)

#endif // DEBUG_UTILS_HPP
