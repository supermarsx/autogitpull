// options_repo_settings.cpp
//
// Parse per-repository override settings from configuration maps.

#include <map>
#include <string>
#include <algorithm>
#include <climits>
#include <cstddef>

#include "options.hpp"
#include "parse_utils.hpp"

namespace fs = std::filesystem;

void parse_repo_settings(Options& opts,
                         const std::map<std::string, std::map<std::string, std::string>>&
                             cfg_repo_opts) {
    bool ok = false;
    for (const auto& [repo, values] : cfg_repo_opts) {
        RepoOptions ro;
        auto rflag = [&](const std::string& k) {
            auto it = values.find(k);
            if (it == values.end())
                return false;
            std::string v = it->second;
            std::transform(v.begin(), v.end(), v.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return v == "" || v == "1" || v == "true" || v == "yes";
        };
        auto ropt = [&](const std::string& k) {
            auto it = values.find(k);
            if (it != values.end())
                return it->second;
            return std::string();
        };
        ok = false;
        if (rflag("--force-pull") || rflag("--discard-dirty"))
            ro.force_pull = true;
        if (rflag("--exclude"))
            ro.exclude = true;
        if (rflag("--check-only"))
            ro.check_only = true;
        if (values.count("--download-limit")) {
            size_t bytes = parse_bytes(ropt("--download-limit"), 0, SIZE_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo download-limit");
            ro.download_limit = bytes / 1024ull;
        }
        if (values.count("--upload-limit")) {
            size_t bytes = parse_bytes(ropt("--upload-limit"), 0, SIZE_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo upload-limit");
            ro.upload_limit = bytes / 1024ull;
        }
        if (values.count("--disk-limit")) {
            size_t bytes = parse_bytes(ropt("--disk-limit"), 0, SIZE_MAX, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo disk-limit");
            ro.disk_limit = bytes / 1024ull;
        }
        if (values.count("--cpu-limit")) {
            double pct = parse_double(ropt("--cpu-limit"), 0.0, 100.0, ok);
            if (!ok)
                throw std::runtime_error("Invalid per-repo cpu-limit");
            ro.cpu_limit = pct;
        }
        if (values.count("--max-runtime")) {
            auto dur = parse_duration(ropt("--max-runtime"), ok);
            if (!ok || dur.count() < 1 || dur.count() > INT_MAX)
                throw std::runtime_error("Invalid per-repo max-runtime");
            ro.max_runtime = dur;
        }
        if (values.count("--pull-timeout")) {
            auto dur = parse_duration(ropt("--pull-timeout"), ok);
            if (!ok || dur.count() < 1 || dur.count() > INT_MAX)
                throw std::runtime_error("Invalid per-repo pull-timeout");
            ro.pull_timeout = dur;
        }
        if (values.count("--post-pull-hook")) {
            ro.post_pull_hook = fs::path(ropt("--post-pull-hook"));
        }
        if (values.count("--pull-ref")) {
            std::string val = ropt("--pull-ref");
            if (val.empty())
                throw std::runtime_error("Invalid per-repo pull-ref");
            ro.pull_ref = val;
        }
        opts.repo_settings[fs::path(repo)] = ro;
    }
}
