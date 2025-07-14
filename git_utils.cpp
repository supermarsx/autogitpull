#include "git_utils.h"
#include <cstdio>
#include <array>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
const char* QUOTE = "\"";
#else
const char* QUOTE = "'";
#endif

namespace git {

std::string run_cmd(const std::string& cmd, int timeout_sec) {
#ifndef _WIN32
    std::string final_cmd = cmd;
    if (timeout_sec > 0) {
        final_cmd = "timeout " + std::to_string(timeout_sec) + " " + cmd;
    }
#else
    std::string final_cmd = cmd;
    (void)timeout_sec;
#endif

    std::string data;
    char buffer[256];
    FILE* stream = popen(final_cmd.c_str(), "r");
    if (stream) {
        while (fgets(buffer, sizeof(buffer), stream)) {
            data.append(buffer);
        }
        pclose(stream);
    }
    if (!data.empty() && data.back() == '\n') data.pop_back();
    return data;
}

bool is_git_repo(const fs::path& p) {
    return fs::exists(p / ".git") && fs::is_directory(p / ".git");
}

std::string get_local_hash(const fs::path& repo) {
    std::string cmd = "cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
        + " && git rev-parse HEAD 2>&1";
    return run_cmd(cmd, 30);
}

std::string get_current_branch(const fs::path& repo) {
    std::string cmd = "cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
        + " && git rev-parse --abbrev-ref HEAD 2>&1";
    return run_cmd(cmd, 30);
}

std::string get_remote_hash(const fs::path& repo, const std::string& branch) {
    std::string fetch_cmd = "cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
        + " && git fetch --quiet 2>&1";
    run_cmd(fetch_cmd, 30);
    std::string cmd = "cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
        + " && git rev-parse origin/" + branch + " 2>&1";
    return run_cmd(cmd, 30);
}

std::string get_origin_url(const fs::path& repo) {
    std::string cmd = "cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
        + " && git config --get remote.origin.url 2>&1";
    return run_cmd(cmd, 15);
}

bool is_github_url(const std::string& url) {
    return url.find("github.com") != std::string::npos;
}

bool remote_accessible(const fs::path& repo) {
    std::string cmd = "cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
        + " && git ls-remote -h origin HEAD 2>&1";
    std::string out = run_cmd(cmd, 30);
    if (out.find("fatal") != std::string::npos || out.find("Permission denied") != std::string::npos
        || out.find("ERROR") != std::string::npos)
        return false;
    return true;
}

int try_pull(const fs::path& repo, std::string& out_pull_log) {
    std::string pull = run_cmd("cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE) + " && git pull 2>&1", 30);
    out_pull_log = pull;
    if (pull.find("package-lock.json") != std::string::npos && pull.find("Please commit your changes or stash them before you merge") != std::string::npos) {
        run_cmd("cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE) + " && git checkout -- package-lock.json", 30);
        pull = run_cmd("cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE) + " && git pull 2>&1", 30);
        out_pull_log += "\n[Auto-discarded package-lock.json]\n" + pull;
        if (pull.find("Already up to date") != std::string::npos || pull.find("Updating") != std::string::npos)
            return 1;
        else
            return 2;
    }
    if (pull.find("Already up to date") != std::string::npos || pull.find("Updating") != std::string::npos)
        return 0;
    return 2;
}

} // namespace git
