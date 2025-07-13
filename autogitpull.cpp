#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
const char* QUOTE = "\"";
#else
const char* QUOTE = "'";
#endif

namespace fs = std::filesystem;

// Run a shell command and get output
std::string run_cmd(const std::string& cmd) {
    std::string data;
    char buffer[256];
    FILE* stream = popen(cmd.c_str(), "r");
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
    return run_cmd(cmd);
}

std::string get_current_branch(const fs::path& repo) {
    std::string cmd = "cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
        + " && git rev-parse --abbrev-ref HEAD 2>&1";
    return run_cmd(cmd);
}

std::string get_remote_hash(const fs::path& repo, const std::string& branch) {
    std::string fetch_cmd = "cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
        + " && git fetch --quiet 2>&1";
    run_cmd(fetch_cmd);
    std::string cmd = "cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
        + " && git rev-parse origin/" + branch + " 2>&1";
    return run_cmd(cmd);
}

void process_repo(const fs::path& repo) {
    std::string branch = get_current_branch(repo);
    if (branch.empty() || branch == "HEAD") {
        std::cout << "[" << repo.string() << "] Detached HEAD or failed to get branch, skipping...\n";
        return;
    }
    std::string local = get_local_hash(repo);
    std::string remote = get_remote_hash(repo, branch);
    if (local.empty() || remote.empty()) {
        std::cout << "[" << repo.string() << "] Error reading hashes (no remote or not on branch?)\n";
        return;
    }
    if (local != remote) {
        std::cout << "[" << repo.string() << "] Remote update detected. Pulling...\n";
        std::string pull = run_cmd("cd " + std::string(QUOTE) + repo.string() + std::string(QUOTE)
                                + " && git pull 2>&1");
        std::cout << pull << std::endl;
    } else {
        std::cout << "[" << repo.string() << "] Up to date.\n";
    }
}

// Portable function to skip nested repos (no disable_recursion_pending)
std::vector<fs::path> find_git_repos(const fs::path& root) {
    std::vector<fs::path> repos;
    for (fs::recursive_directory_iterator it(root), end; it != end; ++it) {
        if (it->is_directory() && is_git_repo(it->path())) {
            repos.push_back(it->path());
            // Skip over all subdirs of this repo
            auto base = it->path();
            ++it;
            while (it != end && it->path().string().find(base.string() + fs::path::preferred_separator) == 0) {
                ++it;
            }
            --it; // for-loop will ++it again
        }
    }
    return repos;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <root-folder>\n";
        return 1;
    }
    fs::path root = argv[1];
    if (!fs::exists(root) || !fs::is_directory(root)) {
        std::cerr << "Root path does not exist or is not a directory.\n";
        return 1;
    }

    while (true) {
        std::vector<fs::path> repos = find_git_repos(root);
        std::cout << "\n--- Found " << repos.size() << " git repos under " << root.string() << " ---\n";
        for (const auto& repo : repos) {
            process_repo(repo);
        }
        std::cout << "--- Sleeping for 60 seconds ---\n";
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
    return 0;
}
