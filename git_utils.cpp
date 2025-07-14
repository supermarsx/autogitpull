#include "git_utils.hpp"
#include <boost/process.hpp>
#include <chrono>
#include <future>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
namespace bp = boost::process;

namespace git {

CmdResult run_cmd(const string& cmd, int timeout_sec) {
    CmdResult result;
    bp::ipstream pipe;
    bp::child c(cmd, bp::shell, (bp::std_out & bp::std_err) > pipe);

    auto reader = async(std::launch::async, [&]() {
        string data, line;
        while (getline(pipe, line)) {
            data += line + '\n';
        }
        return data;
    });

    if (timeout_sec > 0) {
        if (!c.wait_for(chrono::seconds(timeout_sec))) {
            c.terminate();
            c.wait();
            result.exit_code = -1;
            result.output = reader.get();
            if (!result.output.empty() && result.output.back() == '\n')
                result.output.pop_back();
            return result;
        }
    } else {
        c.wait();
    }

    result.output = reader.get();
    if (!result.output.empty() && result.output.back() == '\n')
        result.output.pop_back();
    result.exit_code = c.exit_code();
    return result;
}

string quote_path(const fs::path& p) {
#ifdef _WIN32
    string s = p.string();
    string escaped;
    for (char ch : s) {
        if (ch == '"') escaped += "\\\"";
        else escaped += ch;
    }
    return string("\"") + escaped + string("\"");
#else
    string s = p.string();
    string escaped;
    for (char ch : s) {
        if (ch == '\'' )
            escaped += "'\\''";
        else
            escaped += ch;
    }
    return string("'") + escaped + string("'");
#endif
}

bool is_git_repo(const fs::path& p) {
    return fs::exists(p / ".git") && fs::is_directory(p / ".git");
}

string get_local_hash(const fs::path& repo) {
    string cmd = "cd " + quote_path(repo) + " && git rev-parse HEAD 2>&1";
    return run_cmd(cmd, 30).output;
}

string get_current_branch(const fs::path& repo) {
    string cmd = "cd " + quote_path(repo) + " && git rev-parse --abbrev-ref HEAD 2>&1";
    return run_cmd(cmd, 30).output;
}

string get_remote_hash(const fs::path& repo, const string& branch) {
    string fetch_cmd = "cd " + quote_path(repo) + " && git fetch --quiet 2>&1";
    run_cmd(fetch_cmd, 30);
    string cmd = "cd " + quote_path(repo) + " && git rev-parse origin/" + branch + " 2>&1";
    return run_cmd(cmd, 30).output;
}

string get_origin_url(const fs::path& repo) {
    string cmd = "cd " + quote_path(repo) + " && git config --get remote.origin.url 2>&1";
    return run_cmd(cmd, 15).output;
}

bool is_github_url(const string& url) {
    return url.find("github.com") != string::npos;
}

bool remote_accessible(const fs::path& repo) {
    string cmd = "cd " + quote_path(repo) + " && git ls-remote -h origin HEAD 2>&1";
    string out = run_cmd(cmd, 30).output;
    if (out.find("fatal") != string::npos || out.find("Permission denied") != string::npos ||
        out.find("ERROR") != string::npos)
        return false;
    return true;
}

int try_pull(const fs::path& repo, string& out_pull_log) {
    auto res = run_cmd("cd " + quote_path(repo) + " && git pull 2>&1", 30);
    string pull = res.output;
    out_pull_log = pull;
    if (pull.find("package-lock.json") != string::npos &&
        pull.find("Please commit your changes or stash them before you merge") != string::npos) {
        run_cmd("cd " + quote_path(repo) + " && git checkout -- package-lock.json", 30);
        pull = run_cmd("cd " + quote_path(repo) + " && git pull 2>&1", 30).output;
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
