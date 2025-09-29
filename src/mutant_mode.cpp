#include "mutant_mode.hpp"

#include <fstream>
#include <map>
#include <sstream>

#include "git_utils.hpp"

namespace fs = std::filesystem;

static Options* current_opts = nullptr;
static fs::path cfg_path;
static std::map<fs::path, std::time_t> repo_times;

static fs::path config_path(const Options& opts) {
    if (!opts.mutant_config.empty())
        return opts.mutant_config;
    if (opts.root.empty())
        return fs::path(".autogitpull.mutant");
    return opts.root / ".autogitpull.mutant";
}

static void load_config(Options& opts, const fs::path& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open())
        return;
    int interval = 0;
    long long timeout = 0;
    ifs >> interval >> timeout;
    if (interval > 0)
        opts.interval = interval;
    if (timeout > 0) {
        opts.limits.pull_timeout = std::chrono::seconds(timeout);
        opts.limits.skip_timeout = false;
    }
    std::string line;
    std::getline(ifs, line); // consume rest of first line
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string p;
        long long t = 0;
        if (iss >> p >> t)
            repo_times[p] = static_cast<std::time_t>(t);
    }
}

static void save_config(const Options& opts, const fs::path& path) {
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs.is_open())
        return;
    ofs << opts.interval << " " << opts.limits.pull_timeout.count() << "\n";
    for (const auto& [p, t] : repo_times)
        ofs << p.string() << " " << static_cast<long long>(t) << "\n";
}

void apply_mutant_mode(Options& opts) {
    if (!opts.mutant_mode)
        return;
    current_opts = &opts;
    cfg_path = config_path(opts);
    load_config(opts, cfg_path);
    if (opts.interval < 5)
        opts.interval = 5;
    if (opts.limits.pull_timeout.count() == 0) {
        opts.limits.pull_timeout = std::chrono::seconds(30);
        opts.limits.skip_timeout = false;
    }
    opts.retry_skipped = true;
    opts.skip_unavailable = false;
    if (opts.updated_since.count() == 0)
        opts.updated_since = std::chrono::hours(1);
    save_config(opts, cfg_path);
}

bool mutant_should_pull(const fs::path& repo, RepoInfo& ri, const std::string& remote,
                        bool include_private, std::chrono::seconds updated_since) {
    std::time_t ct =
        git::get_remote_commit_time(repo, remote, ri.branch, include_private, &ri.auth_failed);
    if (ct == 0)
        ct = git::get_last_commit_time(repo);
    std::time_t now = std::time(nullptr);
    if (updated_since.count() > 0 && (ct == 0 || now - ct > updated_since.count())) {
        ri.status = RS_SKIPPED;
        ri.message = "Older than limit";
        return false;
    }
    auto it = repo_times.find(repo);
    if (it != repo_times.end() && it->second == ct) {
        ri.status = RS_SKIPPED;
        ri.message = "No change";
        return false;
    }
    ri.commit_time = ct;
    repo_times[repo] = ct;
    if (current_opts)
        save_config(*current_opts, cfg_path);
    return true;
}

void mutant_record_result(const fs::path& repo, RepoStatus status, std::chrono::seconds duration) {
    (void)repo;
    if (!current_opts)
        return;
    auto& timeout = current_opts->limits.pull_timeout;
    auto old_timeout = timeout;
    if (status == RS_TIMEOUT) {
        timeout += std::chrono::seconds(5);
    } else if ((status == RS_PULL_OK || status == RS_PKGLOCK_FIXED) && duration.count() > 0) {
        if (duration >= timeout) {
            timeout += std::chrono::seconds(5);
        } else if (duration * 2 < timeout && timeout > std::chrono::seconds(10)) {
            timeout -= std::chrono::seconds(5);
        }
    }
    if (timeout != old_timeout)
        save_config(*current_opts, cfg_path);
}
