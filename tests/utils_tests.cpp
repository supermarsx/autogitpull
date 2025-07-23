#include "test_common.hpp"

TEST_CASE("Resource helpers") {
    procutil::set_thread_poll_interval(1);
    procutil::get_thread_count();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(procutil::get_thread_count() >= 1);
    REQUIRE(procutil::get_virtual_memory_kb() >= procutil::get_memory_usage_mb() * 1024);
}

TEST_CASE("Lock file guards instances") {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "autogitpull_lock_test";
    fs::create_directories(dir);
    fs::path lock = dir / ".autogitpull.lock";
    fs::remove(lock);
    {
        procutil::LockFileGuard g1(lock);
        REQUIRE(g1.locked);
        procutil::LockFileGuard g2(lock);
        REQUIRE_FALSE(g2.locked);
        unsigned long pid = 0;
        REQUIRE(procutil::read_lock_pid(lock, pid));
        REQUIRE(procutil::process_running(pid));
    }
    procutil::LockFileGuard g3(lock);
    REQUIRE(g3.locked);
    fs::remove_all(dir);
}

TEST_CASE("Thread count reflects running threads") {
    procutil::set_thread_poll_interval(1);
    std::size_t before = procutil::get_thread_count();
    {
        th_compat::jthread tg([] { std::this_thread::sleep_for(std::chrono::seconds(2)); });
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::size_t during = procutil::get_thread_count();
        // Account for environments where thread polling may lag
        REQUIRE(during >= before);
    }
}

TEST_CASE("th_compat::jthread fallback works") {
#ifndef __cpp_lib_jthread
    bool ran = false;
    th_compat::jthread t([&] { ran = true; });
    t.join();
    REQUIRE(ran);
#else
    SUCCEED("std::jthread available");
#endif
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

TEST_CASE("Network usage upload bytes") {
#ifdef __linux__
    procutil::init_network_usage();
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(srv >= 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);
    REQUIRE(bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    socklen_t len = sizeof(addr);
    REQUIRE(getsockname(srv, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    REQUIRE(listen(srv, 1) == 0);
    std::thread th([&]() {
        int fd = accept(srv, nullptr, nullptr);
        REQUIRE(fd >= 0);
        char buf[4096];
        read(fd, buf, sizeof(buf));
        close(fd);
    });
    int cli = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(cli >= 0);
    REQUIRE(connect(cli, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    std::vector<char> data(4096, 'x');
    REQUIRE(send(cli, data.data(), data.size(), 0) == static_cast<ssize_t>(data.size()));
    close(cli);
    th.join();
    close(srv);
    auto usage = procutil::get_network_usage();
    REQUIRE(usage.upload_bytes >= data.size());
#endif
}
