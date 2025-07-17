#include <catch2/catch_test_macros.hpp>
#include "arg_parser.hpp"
#include "git_utils.hpp"
#include "repo.hpp"
#include "logger.hpp"
#include "resource_utils.hpp"
#include "thread_utils.hpp"
#include "time_utils.hpp"
#include "config_utils.hpp"
#include <filesystem>
#include <thread>
#include <fstream>
#include <cstdlib>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

static std::size_t read_thread_count() {
#ifdef __linux__
    std::ifstream status("/proc/self/status");
    std::string key;
    std::size_t val = 0;
    while (status >> key) {
        if (key == "Threads:") {
            status >> val;
            break;
        }
        std::string rest;
        std::getline(status, rest);
    }
    return val;
#else
    return procutil::get_thread_count();
#endif
}

void scan_repos(const std::vector<fs::path>& all_repos, std::map<fs::path, RepoInfo>& repo_infos,
                std::set<fs::path>& skip_repos, std::mutex& mtx, std::atomic<bool>& scanning_flag,
                std::atomic<bool>& running, std::string& action, std::mutex& action_mtx,
                bool include_private, const fs::path& log_dir, bool check_only, bool hash_check,
                size_t concurrency, int cpu_percent_limit, size_t mem_limit, size_t down_limit,
                size_t up_limit, size_t disk_limit, bool silent, bool force_pull);

std::vector<fs::path> build_repo_list(const fs::path& root, bool recursive,
                                      const std::vector<fs::path>& ignore, size_t max_depth);

TEST_CASE("ArgParser basic parsing") {
    const char* argv[] = {"prog", "--foo", "--opt", "42", "pos", "--unknown"};
    ArgParser parser(6, const_cast<char**>(argv), {"--foo", "--bar", "--opt"});
    REQUIRE(parser.has_flag("--foo"));
    REQUIRE(parser.get_option("--opt") == "42");
    REQUIRE(parser.positional().size() == 1);
    REQUIRE(parser.positional()[0] == "pos");
    REQUIRE(parser.unknown_flags().size() == 1);
    REQUIRE(parser.unknown_flags()[0] == "--unknown");
}

TEST_CASE("ArgParser option with equals") {
    const char* argv[] = {"prog", "--opt=val"};
    ArgParser parser(2, const_cast<char**>(argv), {"--opt"});
    REQUIRE(parser.has_flag("--opt"));
    REQUIRE(parser.get_option("--opt") == "val");
}

TEST_CASE("ArgParser short options") {
    const char* argv[] = {"prog", "-h", "-o", "42"};
    ArgParser parser(4, const_cast<char**>(argv), {"--help", "--opt"},
                     {{'h', "--help"}, {'o', "--opt"}});
    REQUIRE(parser.has_flag("--help"));
    REQUIRE(parser.get_option("--opt") == std::string("42"));
}

TEST_CASE("ArgParser unknown flag detection") {
    const char* argv[] = {"prog", "--foo"};
    ArgParser parser(2, const_cast<char**>(argv), {"--bar"});
    REQUIRE(parser.has_flag("--foo") == false);
    REQUIRE(parser.unknown_flags().size() == 1);
    REQUIRE(parser.unknown_flags()[0] == "--foo");
}

TEST_CASE("Git utils local repo") {
    git::GitInitGuard guard;
    fs::path repo = fs::temp_directory_path() / "git_utils_test_repo";
    fs::remove_all(repo);
    fs::create_directory(repo);
    REQUIRE(git::is_git_repo(repo) == false);

    std::string cmd = "git init " + repo.string() + " > /dev/null 2>&1";
    REQUIRE(std::system(cmd.c_str()) == 0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system(("git -C " + repo.string() + " add file.txt").c_str());
    std::system(("git -C " + repo.string() + " commit -m init > /dev/null 2>&1").c_str());

    REQUIRE(git::is_git_repo(repo));
    std::string branch = git::get_current_branch(repo);
    REQUIRE(!branch.empty());
    std::string hash = git::get_local_hash(repo);
    REQUIRE(hash.size() == 40);
    fs::remove_all(repo);
}

TEST_CASE("Git utils GitHub url detection") {
    REQUIRE(git::is_github_url("https://github.com/user/repo.git"));
    REQUIRE_FALSE(git::is_github_url("https://gitlab.com/user/repo.git"));
}

TEST_CASE("RepoInfo defaults") {
    RepoInfo ri;
    REQUIRE(ri.status == RS_PENDING);
    REQUIRE(ri.progress == 0);
    REQUIRE_FALSE(ri.auth_failed);
}

TEST_CASE("ArgParser log level flags") {
    const char* argv[] = {"prog", "--log-level", "DEBUG", "--verbose"};
    ArgParser parser(4, const_cast<char**>(argv), {"--log-level", "--verbose"});
    REQUIRE(parser.has_flag("--log-level"));
    REQUIRE(parser.get_option("--log-level") == "DEBUG");
    REQUIRE(parser.has_flag("--verbose"));
}

TEST_CASE("ArgParser log file option") {
    const char* argv[] = {"prog", "--log-file", "my.log", "path"};
    ArgParser parser(4, const_cast<char**>(argv), {"--log-file"});
    REQUIRE(parser.has_flag("--log-file"));
    REQUIRE(parser.get_option("--log-file") == "my.log");
    REQUIRE(parser.positional().size() == 1);
    REQUIRE(parser.positional()[0] == "path");
}

TEST_CASE("ArgParser resource flags") {
    const char* argv[] = {"prog", "--max-threads", "4",   "--cpu-percent", "50", "--cpu-cores",
                          "2",    "--mem-limit",   "100", "path"};
    ArgParser parser(10, const_cast<char**>(argv),
                     {"--max-threads", "--cpu-percent", "--cpu-cores", "--mem-limit"});
    REQUIRE(parser.get_option("--max-threads") == std::string("4"));
    REQUIRE(parser.get_option("--cpu-percent") == std::string("50"));
    REQUIRE(parser.get_option("--cpu-cores") == std::string("2"));
    REQUIRE(parser.get_option("--mem-limit") == std::string("100"));
    REQUIRE(parser.positional().size() == 1);
    REQUIRE(parser.positional()[0] == std::string("path"));
}

TEST_CASE("Resource helpers") {
    procutil::set_thread_poll_interval(1);
    procutil::get_thread_count();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(procutil::get_thread_count() >= 1);
    REQUIRE(procutil::get_virtual_memory_kb() >= procutil::get_memory_usage_mb() * 1024);
}

TEST_CASE("Thread count reflects running threads") {
    procutil::set_thread_poll_interval(1);
    std::size_t before = procutil::get_thread_count();
    {
        ThreadGuard tg(std::thread([] { std::this_thread::sleep_for(std::chrono::seconds(2)); }));
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::size_t during = procutil::get_thread_count();
        // Account for environments where thread polling may lag
        REQUIRE(during >= before);
    }
}

TEST_CASE("Logger appends messages") {
    fs::path log = fs::temp_directory_path() / "autogitpull_logger_test.log";
    fs::remove(log);
    init_logger(log.string());
    REQUIRE(logger_initialized());
    log_info("first entry");
    log_error("second entry");
    {
        std::ifstream ifs(log);
        REQUIRE(ifs.good());
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(ifs, line))
            lines.push_back(line);
        REQUIRE(lines.size() >= 2);
        REQUIRE(lines[lines.size() - 2].find("first entry") != std::string::npos);
        REQUIRE(lines.back().find("second entry") != std::string::npos);
    }
    size_t count_before;
    {
        std::ifstream ifs(log);
        std::string l;
        std::vector<std::string> lines;
        while (std::getline(ifs, l))
            lines.push_back(l);
        count_before = lines.size();
    }
    log_info("third entry");
    {
        std::ifstream ifs(log);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(ifs, line))
            lines.push_back(line);
        REQUIRE(lines.size() == count_before + 1);
        REQUIRE(lines.back().find("third entry") != std::string::npos);
    }
    shutdown_logger();
}

TEST_CASE("--log-file without value creates file") {
    const char* argv[] = {"prog", "--log-file"};
    ArgParser parser(2, const_cast<char**>(argv), {"--log-file"});
    REQUIRE(parser.has_flag("--log-file"));
    REQUIRE(parser.get_option("--log-file").empty());

    std::string ts = timestamp();
    for (char& ch : ts) {
        if (ch == ' ' || ch == ':')
            ch = '_';
        else if (ch == '/')
            ch = '-';
    }
    fs::path log = "autogitpull-logs-" + ts + ".log";
    fs::remove(log);
    init_logger(log.string());
    REQUIRE(logger_initialized());
    log_info("test entry");
    shutdown_logger();
    REQUIRE(fs::exists(log));
    fs::remove(log);
}

TEST_CASE("ArgParser threads flags") {
    const char* argv[] = {"prog", "--threads", "8", "--single-thread"};
    ArgParser parser(4, const_cast<char**>(argv), {"--threads", "--single-thread"});
    REQUIRE(parser.has_flag("--threads"));
    REQUIRE(parser.get_option("--threads") == std::string("8"));
    REQUIRE(parser.has_flag("--single-thread"));
}

TEST_CASE("ArgParser network limits") {
    const char* argv[] = {"prog", "--download-limit", "100", "--upload-limit", "50"};
    ArgParser parser(5, const_cast<char**>(argv), {"--download-limit", "--upload-limit"});
    REQUIRE(parser.get_option("--download-limit") == std::string("100"));
    REQUIRE(parser.get_option("--upload-limit") == std::string("50"));
}

TEST_CASE("ArgParser disk limit") {
    const char* argv[] = {"prog", "--disk-limit", "250"};
    ArgParser parser(3, const_cast<char**>(argv), {"--disk-limit"});
    REQUIRE(parser.get_option("--disk-limit") == std::string("250"));
}

TEST_CASE("ArgParser debug flags") {
    const char* argv[] = {"prog", "--debug-memory", "--dump-state", "--dump-large", "5"};
    ArgParser parser(5, const_cast<char**>(argv),
                     {"--debug-memory", "--dump-state", "--dump-large"});
    REQUIRE(parser.has_flag("--debug-memory"));
    REQUIRE(parser.has_flag("--dump-state"));
    REQUIRE(parser.get_option("--dump-large") == std::string("5"));
}

TEST_CASE("ArgParser recursive flag") {
    const char* argv[] = {"prog", "--recursive"};
    ArgParser parser(2, const_cast<char**>(argv), {"--recursive"});
    REQUIRE(parser.has_flag("--recursive"));
}

TEST_CASE("YAML config loading") {
    fs::path cfg = fs::temp_directory_path() / "cfg.yaml";
    {
        std::ofstream ofs(cfg);
        ofs << "interval: 42\n";
        ofs << "cli: true\n";
    }
    std::map<std::string, std::string> opts;
    std::string err;
    REQUIRE(load_yaml_config(cfg.string(), opts, err));
    REQUIRE(opts["--interval"] == "42");
    REQUIRE(opts["--cli"] == "true");
    fs::remove(cfg);
}

TEST_CASE("JSON config loading") {
    fs::path cfg = fs::temp_directory_path() / "cfg.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"interval\": 42, \n  \"cli\": true\n}";
    }
    std::map<std::string, std::string> opts;
    std::string err;
    REQUIRE(load_json_config(cfg.string(), opts, err));
    REQUIRE(opts["--interval"] == "42");
    REQUIRE(opts["--cli"] == "true");
    fs::remove(cfg);
}

TEST_CASE("build_repo_list ignores directories") {
    fs::path root = fs::temp_directory_path() / "ignore_test";
    fs::remove_all(root);
    fs::create_directories(root / "a");
    fs::create_directories(root / "b");
    fs::create_directories(root / "c");

    std::vector<fs::path> ignore{root / "b", root / "c"};
    std::vector<fs::path> repos = build_repo_list(root, false, ignore, 0);
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "b") == repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "c") == repos.end());

    fs::remove_all(root);
}

TEST_CASE("recursive iterator finds nested repo") {
    git::GitInitGuard guard;
    fs::path root = fs::temp_directory_path() / "recursive_test";
    fs::remove_all(root);
    fs::create_directories(root / "sub/inner/nested");
    fs::path repo = root / "sub/inner/nested";
    REQUIRE(std::system(("git init " + repo.string() + " > /dev/null 2>&1").c_str()) == 0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());
    std::ofstream(repo / "file.txt") << "hello";
    std::system(("git -C " + repo.string() + " add file.txt").c_str());
    std::system(("git -C " + repo.string() + " commit -m init > /dev/null 2>&1").c_str());

    std::vector<fs::path> flat;
    for (const auto& e : fs::directory_iterator(root))
        flat.push_back(e.path());
    REQUIRE(std::find(flat.begin(), flat.end(), repo) == flat.end());

    std::vector<fs::path> rec;
    for (const auto& e : fs::recursive_directory_iterator(root))
        if (e.is_directory())
            rec.push_back(e.path());
    REQUIRE(std::find(rec.begin(), rec.end(), repo) != rec.end());

    fs::remove_all(root);
}

TEST_CASE("build_repo_list respects max depth") {
    fs::path root = fs::temp_directory_path() / "depth_test";
    fs::remove_all(root);
    fs::create_directories(root / "a/b/c");

    std::vector<fs::path> repos = build_repo_list(root, true, {}, 2);
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a/b") != repos.end());
    REQUIRE(std::find(repos.begin(), repos.end(), root / "a/b/c") == repos.end());

    fs::remove_all(root);
}

TEST_CASE("scan_repos respects concurrency limit") {
    git::GitInitGuard guard;
    std::vector<fs::path> repos;
    for (int i = 0; i < 3; ++i) {
        fs::path repo = fs::temp_directory_path() / ("threads_repo_" + std::to_string(i));
        fs::remove_all(repo);
        fs::create_directory(repo);
        REQUIRE(std::system(("git init " + repo.string() + " > /dev/null 2>&1").c_str()) == 0);
        std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
        std::system(("git -C " + repo.string() + " config user.name tester").c_str());
        std::ofstream(repo / "file.txt") << "hello";
        std::system(("git -C " + repo.string() + " add file.txt").c_str());
        std::system(("git -C " + repo.string() + " commit -m init > /dev/null 2>&1").c_str());
        repos.push_back(repo);
    }

    std::map<fs::path, RepoInfo> infos;
    for (const auto& p : repos)
        infos[p] = RepoInfo{p, RS_PENDING, "", "", "", "", 0, false};
    std::set<fs::path> skip;
    std::mutex mtx;
    std::atomic<bool> scanning(true);
    std::atomic<bool> running(true);
    std::string act;
    std::mutex act_mtx;

    const std::size_t concurrency = 2;
    std::size_t baseline = read_thread_count();
    std::size_t max_seen = baseline;

    std::thread t([&]() {
        scan_repos(repos, infos, skip, mtx, scanning, running, act, act_mtx, false, fs::path(),
                   true, true, concurrency, 0, 0, 0, 0, 0, true, false);
    });
    while (scanning) {
        max_seen = std::max(max_seen, read_thread_count());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    t.join();

    REQUIRE(max_seen - baseline <= concurrency);

    for (const auto& p : repos)
        fs::remove_all(p);
}

TEST_CASE("try_pull handles dirty repos") {
    git::GitInitGuard guard;
    fs::path remote = fs::temp_directory_path() / "pull_remote.git";
    fs::path src = fs::temp_directory_path() / "pull_src";
    fs::path repo = fs::temp_directory_path() / "pull_work";
    fs::remove_all(remote);
    fs::remove_all(src);
    fs::remove_all(repo);
    REQUIRE(std::system(("git init --bare " + remote.string() + " > /dev/null 2>&1").c_str()) == 0);
    REQUIRE(std::system(("git clone " + remote.string() + " " + src.string() + " > /dev/null 2>&1")
                            .c_str()) == 0);
    std::system(("git -C " + src.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + src.string() + " config user.name tester").c_str());
    std::ofstream(src / "file.txt") << "hello";
    std::system(("git -C " + src.string() + " add file.txt").c_str());
    std::system(("git -C " + src.string() + " commit -m init > /dev/null 2>&1").c_str());
    std::system(("git -C " + src.string() + " push origin master > /dev/null 2>&1").c_str());
    REQUIRE(std::system(("git clone " + remote.string() + " " + repo.string() + " > /dev/null 2>&1")
                            .c_str()) == 0);
    std::system(("git -C " + repo.string() + " config user.email you@example.com").c_str());
    std::system(("git -C " + repo.string() + " config user.name tester").c_str());
    std::ofstream(src / "file.txt", std::ios::app) << "update";
    std::system(("git -C " + src.string() + " add file.txt").c_str());
    std::system(("git -C " + src.string() + " commit -m update > /dev/null 2>&1").c_str());
    std::system(("git -C " + src.string() + " push origin master > /dev/null 2>&1").c_str());
    std::ofstream(repo / "file.txt", std::ios::app) << "local";

    std::string log;
    bool auth_fail = false;
    int ret = git::try_pull(repo, log, nullptr, false, &auth_fail, 0, 0, 0, false);
    REQUIRE(ret == 3);
    REQUIRE(fs::exists(repo / "file.txt"));
    std::string after = git::get_local_hash(repo);
    REQUIRE(after != git::get_local_hash(src));

    ret = git::try_pull(repo, log, nullptr, false, &auth_fail, 0, 0, 0, true);
    REQUIRE(ret == 0);
    {
        std::ifstream ifs(repo / "file.txt");
        std::string contents;
        std::getline(ifs, contents);
        REQUIRE(contents == "helloupdate");
    }
    REQUIRE(git::get_local_hash(repo) == git::get_local_hash(src));

    fs::remove_all(remote);
    fs::remove_all(src);
    fs::remove_all(repo);
}
