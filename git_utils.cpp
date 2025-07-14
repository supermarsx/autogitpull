#include "git_utils.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
const char* QUOTE = "\"";
#else
const char* QUOTE = "'";
#endif

using namespace std;

namespace git {

string run_cmd(const string& cmd, int timeout_sec) {
#ifndef _WIN32
    string final_cmd = cmd;
    if (timeout_sec > 0) {
        final_cmd = "timeout " + to_string(timeout_sec) + " " + cmd;
    }
#else
    string final_cmd = cmd;
    (void)timeout_sec;
#endif
    string data;
    char buffer[256];
    FILE* raw = popen(final_cmd.c_str(), "r");
    if (!raw) return data;
    struct PopenHandle { FILE* fp; ~PopenHandle(){ if(fp) pclose(fp); } } handle{raw};
    while (fgets(buffer, sizeof(buffer), handle.fp)) {
        data.append(buffer);
    }
    if (!data.empty() && data.back() == '\n')
        data.pop_back();
    return data;
}

string quote_path(const fs::path& p) {
    return string(QUOTE) + p.string() + string(QUOTE);
}

bool is_git_repo(const fs::path& p) {
    return fs::exists(p / ".git") && fs::is_directory(p / ".git");
}

string get_local_hash(const fs::path& repo) {
    string cmd = "cd " + quote_path(repo) + " && git rev-parse HEAD 2>&1";
    return run_cmd(cmd, 30);
}

string get_current_branch(const fs::path& repo) {
    string cmd = "cd " + quote_path(repo) + " && git rev-parse --abbrev-ref HEAD 2>&1";
    return run_cmd(cmd, 30);
}

string get_remote_hash(const fs::path& repo, const string& branch) {
    string fetch_cmd = "cd " + quote_path(repo) + " && git fetch --quiet 2>&1";
    run_cmd(fetch_cmd, 30);
    string cmd = "cd " + quote_path(repo) + " && git rev-parse origin/" + branch + " 2>&1";
    return run_cmd(cmd, 30);
}

string get_origin_url(const fs::path& repo) {
    string cmd = "cd " + quote_path(repo) + " && git config --get remote.origin.url 2>&1";
    return run_cmd(cmd, 15);
}

bool is_github_url(const string& url) {
    return url.find("github.com") != string::npos;
}

bool remote_accessible(const fs::path& repo) {
    string cmd = "cd " + quote_path(repo) + " && git ls-remote -h origin HEAD 2>&1";
    string out = run_cmd(cmd, 30);
    if (out.find("fatal") != string::npos || out.find("Permission denied") != string::npos ||
        out.find("ERROR") != string::npos)
        return false;
    return true;
}

int try_pull(const fs::path& repo, string& out_pull_log) {
    string pull = run_cmd("cd " + quote_path(repo) + " && git pull 2>&1", 30);
    out_pull_log = pull;
    if (pull.find("package-lock.json") != string::npos &&
        pull.find("Please commit your changes or stash them before you merge") != string::npos) {
        run_cmd("cd " + quote_path(repo) + " && git checkout -- package-lock.json", 30);
        pull = run_cmd("cd " + quote_path(repo) + " && git pull 2>&1", 30);
        out_pull_log += "\n[Auto-discarded package-lock.json]\n" + pull;
        if (pull.find("Already up to date") != string::npos || pull.find("Updating") != string::npos)
            return 1;
        else
            return 2;
    }
    if (pull.find("Already up to date") != string::npos || pull.find("Updating") != string::npos)
        return 0;
    return 2;
}

} // namespace git
