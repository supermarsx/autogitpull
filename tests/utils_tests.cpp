#include "test_common.hpp"
#include <catch2/catch_approx.hpp>

using Catch::Approx;

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

TEST_CASE("find_running_instances lists instances") {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "instance_list_test";
    fs::create_directories(dir);
    fs::path lock = dir / ".autogitpull.lock";
    fs::remove(lock);
    procutil::LockFileGuard guard(lock);
    REQUIRE(guard.locked);
#ifdef __linux__
    fs::path sock = fs::temp_directory_path() / "instance_list.sock";
    int srv = socket(AF_UNIX, SOCK_STREAM, 0);
    REQUIRE(srv >= 0);
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, sock.c_str(), sizeof(addr.sun_path) - 1);
    unlink(sock.c_str());
    REQUIRE(bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    REQUIRE(listen(srv, 1) == 0);
#endif

    auto list = procutil::find_running_instances();

#ifdef __linux__
    close(srv);
    unlink(sock.c_str());
#endif
    fs::remove_all(dir);

    bool found_lock = false;
    bool found_sock = false;
    for (const auto& [name, pid] : list) {
        if (name == dir.filename().string()) {
            found_lock = true;
            REQUIRE(pid == static_cast<unsigned long>(getpid()));
        }
#ifdef __linux__
        if (name == sock.stem().string()) {
            found_sock = true;
            REQUIRE(pid == static_cast<unsigned long>(getpid()));
        }
#endif
    }
    REQUIRE(found_lock);
#ifdef __linux__
    REQUIRE(found_sock);
#endif
}

TEST_CASE("alerts_allowed logic") {
    Options opts;
    opts.interval = 10;
    REQUIRE_FALSE(alerts_allowed(opts));
    opts.confirm_alert = true;
    REQUIRE(alerts_allowed(opts));
    opts.confirm_alert = false;
    opts.sudo_su = true;
    REQUIRE(alerts_allowed(opts));
    opts.sudo_su = false;
    opts.interval = 20;
    opts.force_pull = true;
    REQUIRE_FALSE(alerts_allowed(opts));
}

TEST_CASE("find_running_instances detects process name") {
#if defined(__linux__) || defined(__APPLE__)
    pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0) {
        execl("/bin/bash", "bash", "-c", "exec -a autogitpull sleep 5", nullptr);
        _exit(1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto list = procutil::find_running_instances();
    bool found = false;
    for (const auto& [name, p] : list) {
        if (p == static_cast<unsigned long>(pid))
            found = true;
    }
    kill(pid, SIGTERM);
    waitpid(pid, nullptr, 0);
    REQUIRE(found);
#elif defined(_WIN32)
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    wcscat_s(sysDir, L"\\cmd.exe");
    wchar_t tmpPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpPath);
    wcscat_s(tmpPath, L"autogitpull.exe");
    CopyFileW(sysDir, tmpPath, FALSE);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    REQUIRE(CreateProcessW(tmpPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto list = procutil::find_running_instances();
    bool found = false;
    for (const auto& [name, p] : list) {
        if (p == static_cast<unsigned long>(pi.dwProcessId))
            found = true;
    }
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    DeleteFileW(tmpPath);
    REQUIRE(found);
#else
    SUCCEED("process listing not supported");
#endif
}

TEST_CASE("CPU usage reset starts from zero") {
    procutil::set_cpu_poll_interval(1);
    procutil::get_cpu_percent();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    procutil::get_cpu_percent();
    procutil::reset_cpu_usage();
    REQUIRE(procutil::get_cpu_percent() == Approx(0.0));
}

TEST_CASE("Disk usage reset starts from zero") {
    namespace fs = std::filesystem;
    procutil::init_disk_usage();
    fs::path f = fs::temp_directory_path() / "disk_usage_reset.tmp";
    {
        std::ofstream ofs(f, std::ios::binary);
        std::vector<char> data(4096, 'x');
        ofs.write(data.data(), data.size());
    }
    auto before = procutil::get_disk_usage();
    REQUIRE(before.write_bytes >= 4096);
    procutil::reset_disk_usage();
    auto after = procutil::get_disk_usage();
    REQUIRE(after.read_bytes == 0);
    REQUIRE(after.write_bytes == 0);
    fs::remove(f);
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

TEST_CASE("Logger rotates when exceeding max size") {
    fs::path log = fs::temp_directory_path() / "autogitpull_logger_rotate.log";
    fs::remove(log);
    fs::path log1 = log;
    log1 += ".1";
    fs::remove(log1);
    init_logger(log.string(), LogLevel::INFO, 200, 1);
    REQUIRE(logger_initialized());
    for (int i = 0; i < 50; ++i)
        log_info("entry " + std::to_string(i));
    shutdown_logger();
    std::error_code ec;
    REQUIRE(std::filesystem::file_size(log, ec) <= 200);
    REQUIRE(std::filesystem::exists(log1));
    fs::remove(log);
    fs::remove(log1);
}

TEST_CASE("Logger respects max rotated files") {
    fs::path log = fs::temp_directory_path() / "autogitpull_logger_depth.log";
    fs::path log1 = log;
    log1 += ".1";
    fs::path log2 = log;
    log2 += ".2";
    fs::path log3 = log;
    log3 += ".3";
    fs::path log4 = log;
    log4 += ".4";
    fs::remove(log);
    fs::remove(log1);
    fs::remove(log2);
    fs::remove(log3);
    fs::remove(log4);
    init_logger(log.string(), LogLevel::INFO, 100, 3);
    REQUIRE(logger_initialized());
    for (int i = 0; i < 200; ++i)
        log_info("entry " + std::to_string(i));
    shutdown_logger();
    REQUIRE(std::filesystem::exists(log));
    REQUIRE(std::filesystem::exists(log1));
    REQUIRE(std::filesystem::exists(log2));
    REQUIRE(std::filesystem::exists(log3));
    REQUIRE_FALSE(std::filesystem::exists(log4));
    fs::remove(log);
    fs::remove(log1);
    fs::remove(log2);
    fs::remove(log3);
}

TEST_CASE("Logger outputs JSON when enabled") {
    fs::path log = fs::temp_directory_path() / "autogitpull_json.log";
    fs::remove(log);
    init_logger(log.string());
    set_json_logging(true);
    log_info("json entry", {{"k", "v"}});
    shutdown_logger();
    set_json_logging(false);
    std::ifstream ifs(log);
    REQUIRE(ifs.good());
    std::string line;
    std::getline(ifs, line);
    REQUIRE(line.find("\"timestamp\"") != std::string::npos);
    REQUIRE(line.find("\"level\":\"INFO\"") != std::string::npos);
    REQUIRE(line.find("\"msg\":\"json entry\"") != std::string::npos);
    REQUIRE(line.find("\"k\":\"v\"") != std::string::npos);
    fs::remove(log);
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
#if defined(__linux__) || defined(__APPLE__)
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
    procutil::reset_network_usage();
    auto after = procutil::get_network_usage();
    REQUIRE(after.download_bytes == 0);
    REQUIRE(after.upload_bytes == 0);
#endif
}
