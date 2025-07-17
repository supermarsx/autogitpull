#include "debug_utils.hpp"
#include <sstream>

namespace debug_utils {

void dump_repo_infos(const std::map<std::filesystem::path, RepoInfo>& infos) {
    std::ostringstream oss;
    oss << "repo_infos(" << infos.size() << ")";
    for (const auto& [p, info] : infos) {
        oss << "\n"
            << p.string() << " status=" << static_cast<int>(info.status) << " msg=" << info.message;
    }
    log_debug(oss.str());
}

} // namespace debug_utils
