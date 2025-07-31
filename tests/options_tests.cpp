#include "test_common.hpp"

TEST_CASE("parse_options service flags") {
    const char* argv[] = {"prog", "path",     "--install-service", "--service-config",
                          "cfg",  "--persist"};
    Options opts = parse_options(6, const_cast<char**>(argv));
    REQUIRE(opts.install_service);
    REQUIRE(opts.service_config == std::string("cfg"));
    REQUIRE(opts.persist);
    REQUIRE(opts.attach_name == std::string("path"));
    const char* argv2[] = {"prog", "path", "--uninstall-service"};
    Options opts2 = parse_options(3, const_cast<char**>(argv2));
    REQUIRE(opts2.uninstall_service);
}

TEST_CASE("parse_options service control flags") {
    const char* argv[] = {"prog", "path", "--start-service", "--stop-service", "--restart-service"};
    Options opts = parse_options(5, const_cast<char**>(argv));
    REQUIRE(opts.start_service);
    REQUIRE(opts.stop_service);
    REQUIRE(opts.restart_service);
}

TEST_CASE("parse_options daemon control flags") {
    const char* argv[] = {"prog", "path", "--start-daemon", "--stop-daemon", "--restart-daemon"};
    Options opts = parse_options(5, const_cast<char**>(argv));
    REQUIRE(opts.start_daemon);
    REQUIRE(opts.stop_daemon);
    REQUIRE(opts.restart_daemon);
}

TEST_CASE("parse_options service name flags") {
    const char* argv[] = {"prog", "path", "--service-name", "svc"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.service_name == std::string("svc"));
}

TEST_CASE("parse_options daemon name flag") {
    const char* argv[] = {"prog", "path", "--daemon-name", "dname"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.daemon_name == std::string("dname"));
}

TEST_CASE("parse_options show service flag") {
    const char* argv[] = {"prog", "path", "--show-service"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.show_service);
}

TEST_CASE("parse_options service status flag") {
    const char* argv[] = {"prog", "path", "--service-status"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service_status);
}

TEST_CASE("parse_options daemon status flag") {
    const char* argv[] = {"prog", "path", "--daemon-status"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.daemon_status);
}

TEST_CASE("parse_options list services flags") {
    const char* argv[] = {"prog", "path", "--list-services"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.list_services);
    const char* argv2[] = {"prog", "path", "--list-daemons"};
    Options opts2 = parse_options(3, const_cast<char**>(argv2));
    REQUIRE(opts2.list_services);
}

TEST_CASE("parse_options attach option") {
    const char* argv[] = {"prog", "--attach", "foo"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.attach_name == std::string("foo"));
}

TEST_CASE("parse_options background option") {
    const char* argv[] = {"prog", "path", "--background", "foo"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.run_background);
    REQUIRE(opts.attach_name == std::string("foo"));
}

TEST_CASE("parse_options reattach option") {
    const char* argv[] = {"prog", "--reattach", "foo"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.reattach);
    REQUIRE(opts.attach_name == std::string("foo"));
}

TEST_CASE("parse_options persist name option") {
    const char* argv[] = {"prog", "path", "--persist=myrun"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.persist);
    REQUIRE(opts.attach_name == std::string("myrun"));
}

TEST_CASE("parse_options runtime options") {
    const char* argv[] = {"prog", "path", "--show-runtime", "--max-runtime", "5"};
    Options opts = parse_options(5, const_cast<char**>(argv));
    REQUIRE(opts.show_runtime);
    REQUIRE(opts.runtime_limit == std::chrono::seconds(5));
}

TEST_CASE("parse_options kill-all option") {
    const char* argv[] = {"prog", "path", "--kill-all"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.kill_all);
}

TEST_CASE("parse_options kill-on-sleep option") {
    const char* argv[] = {"prog", "path", "--kill-on-sleep"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.kill_on_sleep);
}

TEST_CASE("parse_options rescan-new option default") {
    const char* argv[] = {"prog", "path", "--rescan-new"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.rescan_new);
    REQUIRE(opts.rescan_interval == std::chrono::minutes(5));
}

TEST_CASE("parse_options rescan-new custom interval") {
    const char* argv[] = {"prog", "path", "--rescan-new", "10"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.rescan_new);
    REQUIRE(opts.rescan_interval == std::chrono::minutes(10));
}

TEST_CASE("parse_options commit options") {
    const char* argv[] = {"prog",
                          "path",
                          "--show-commit-date",
                          "--show-commit-author",
                          "--vmem",
                          "--no-colors",
                          "--color",
                          "\033[31m",
                          "--row-order",
                          "alpha",
                          "--syslog",
                          "--syslog-facility",
                          "5"};
    Options opts = parse_options(13, const_cast<char**>(argv));
    REQUIRE(opts.show_commit_date);
    REQUIRE(opts.show_commit_author);
    REQUIRE(opts.show_vmem);
    REQUIRE(opts.no_colors);
    REQUIRE(opts.custom_color == "\033[31m");
    REQUIRE(opts.sort_mode == Options::ALPHA);
    REQUIRE(opts.use_syslog);
    REQUIRE(opts.syslog_facility == 5);
}

TEST_CASE("parse_options single repo") {
    const char* argv[] = {"prog", "path", "--single-repo"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.single_repo);
}

TEST_CASE("parse_options short flags") {
    const char* argv[] = {"prog", "path", "-x", "-m", "-w", "-X"};
    Options opts = parse_options(6, const_cast<char**>(argv));
    REQUIRE(opts.check_only);
    REQUIRE(opts.debug_memory);
    REQUIRE(opts.rescan_new);
    REQUIRE_FALSE(opts.cpu_tracker);
    REQUIRE(opts.rescan_interval == std::chrono::minutes(5));
}

TEST_CASE("parse_options dont skip timeouts") {
    const char* argv[] = {"prog", "path", "--dont-skip-timeouts"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE_FALSE(opts.skip_timeout);
}

TEST_CASE("parse_options pull timeout") {
    const char* argv[] = {"prog", "path", "--pull-timeout", "60"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.pull_timeout == std::chrono::seconds(60));
}

TEST_CASE("parse_options keep first valid") {
    const char* argv[] = {"prog", "path", "--keep-first-valid"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.keep_first_valid);
}

TEST_CASE("parse_options show repo count") {
    const char* argv[] = {"prog", "path", "--show-repo-count"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.show_repo_count);
}

TEST_CASE("parse_options print skipped and pull author") {
    const char* argv[] = {"prog", "path", "--print-skipped", "--show-pull-author"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.cli_print_skipped);
    REQUIRE(opts.show_pull_author);
}

TEST_CASE("parse_options keep first alias") {
    const char* argv[] = {"prog", "path", "--keep-first"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.keep_first_valid);
}

TEST_CASE("parse_options ignore lock") {
    const char* argv[] = {"prog", "path", "--ignore-lock"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.ignore_lock);
}

TEST_CASE("parse_options enable hotkeys") {
    const char* argv[] = {"prog", "path", "--enable-hotkeys"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.enable_hotkeys);
}

TEST_CASE("parse_options repo overrides") {
    fs::path cfg = fs::temp_directory_path() / "opts_repo.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"root\": \"/tmp\",\n  \"/tmp/repo\": {\n    \"force-pull\": true\n  }\n}";
    }
    std::string p = cfg.string();
    char* path_c = strdup(p.c_str());
    char* argv[] = {const_cast<char*>("prog"), const_cast<char*>("--config-json"), path_c};
    Options opts = parse_options(3, argv);
    free(path_c);
    REQUIRE(opts.root == fs::path("/tmp"));
    REQUIRE(opts.repo_overrides.count(fs::path("/tmp/repo")) == 1);
    REQUIRE(opts.repo_overrides[fs::path("/tmp/repo")].force_pull.value_or(false));
    fs::remove(cfg);
}

struct DirGuard {
    std::filesystem::path old;
    explicit DirGuard(const std::filesystem::path& p) : old(std::filesystem::current_path()) { std::filesystem::current_path(p); }
    ~DirGuard() { std::filesystem::current_path(old); }
};

TEST_CASE("parse_options auto-config root directory") {
    fs::path root_dir = fs::temp_directory_path() / "auto_cfg_root";
    fs::path cwd_dir = fs::temp_directory_path() / "auto_cfg_cwd";
    fs::path exe_dir = fs::temp_directory_path() / "auto_cfg_exe";
    fs::create_directories(root_dir);
    fs::create_directories(cwd_dir);
    fs::create_directories(exe_dir);
    std::ofstream(root_dir / ".autogitpull.yaml") << "interval: 1\n";
    std::ofstream(cwd_dir / ".autogitpull.yaml") << "interval: 2\n";
    std::ofstream(exe_dir / ".autogitpull.yaml") << "interval: 3\n";
    DirGuard guard(cwd_dir);
    std::string exe = (exe_dir / "prog").string();
    std::string root_s = root_dir.string();
    char* argv[] = {const_cast<char*>(exe.c_str()), const_cast<char*>(root_s.c_str()), const_cast<char*>("--auto-config")};
    Options opts = parse_options(3, argv);
    fs::remove_all(root_dir);
    fs::remove_all(cwd_dir);
    fs::remove_all(exe_dir);
    REQUIRE(opts.interval == 1);
}

TEST_CASE("parse_options auto-config cwd fallback") {
    fs::path root_dir = fs::temp_directory_path() / "auto_cfg_root2";
    fs::path cwd_dir = fs::temp_directory_path() / "auto_cfg_cwd2";
    fs::path exe_dir = fs::temp_directory_path() / "auto_cfg_exe2";
    fs::create_directories(root_dir);
    fs::create_directories(cwd_dir);
    fs::create_directories(exe_dir);
    std::ofstream(cwd_dir / ".autogitpull.yaml") << "interval: 2\n";
    std::ofstream(exe_dir / ".autogitpull.yaml") << "interval: 3\n";
    DirGuard guard(cwd_dir);
    std::string exe = (exe_dir / "prog").string();
    std::string root_s = root_dir.string();
    char* argv[] = {const_cast<char*>(exe.c_str()), const_cast<char*>(root_s.c_str()), const_cast<char*>("--auto-config")};
    Options opts = parse_options(3, argv);
    fs::remove_all(root_dir);
    fs::remove_all(cwd_dir);
    fs::remove_all(exe_dir);
    REQUIRE(opts.interval == 2);
}

TEST_CASE("parse_options auto-config exe directory fallback") {
    fs::path root_dir = fs::temp_directory_path() / "auto_cfg_root3";
    fs::path cwd_dir = fs::temp_directory_path() / "auto_cfg_cwd3";
    fs::path exe_dir = fs::temp_directory_path() / "auto_cfg_exe3";
    fs::create_directories(root_dir);
    fs::create_directories(cwd_dir);
    fs::create_directories(exe_dir);
    std::ofstream(exe_dir / ".autogitpull.yaml") << "interval: 3\n";
    DirGuard guard(cwd_dir);
    std::string exe = (exe_dir / "prog").string();
    std::string root_s = root_dir.string();
    char* argv[] = {const_cast<char*>(exe.c_str()), const_cast<char*>(root_s.c_str()), const_cast<char*>("--auto-config")};
    Options opts = parse_options(3, argv);
    fs::remove_all(root_dir);
    fs::remove_all(cwd_dir);
    fs::remove_all(exe_dir);
    REQUIRE(opts.interval == 3);
}
