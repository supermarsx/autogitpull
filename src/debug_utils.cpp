#include "debug_utils.hpp"
#include <sstream>

namespace debug_utils {

void log_memory_delta_mb(std::size_t current, std::size_t& last) {
    long long delta = static_cast<long long>(current) - static_cast<long long>(last);
    log_debug("Memory=" + std::to_string(current) + "MB delta=" + std::to_string(delta) + "MB");
    last = current;
}

void dump_repo_infos(const std::map<std::filesystem::path, RepoInfo>& infos,
                     std::size_t max_items) {
    std::ostringstream oss;
    oss << "repo_infos(" << infos.size() << ")";
    std::size_t count = 0;
    for (const auto& [p, info] : infos) {
        if (count++ >= max_items) {
            oss << "\n...";
            break;
        }
        oss << "\n"
            << p.string() << " status=" << static_cast<int>(info.status) << " msg=" << info.message;
    }
    log_debug(oss.str());
}

} // namespace debug_utils
