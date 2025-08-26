#include "test_common.hpp"
#include "git_utils.hpp"

namespace git {
int credential_cb(git_credential** out, const char* url, const char* username_from_url,
                  unsigned int allowed_types, void* payload);
}

TEST_CASE("parse_service_options start service flag") {
    const char* argv[] = {"prog", "--service-name", "svc", "--start-service"};
    ArgParser parser(4, const_cast<char**>(argv));
    Options opts;
    auto cfg_flag = [](const std::string&) { return false; };
    auto cfg_opt = [](const std::string&) -> std::string { return ""; };
    std::map<std::string, std::string> cfg_opts;
    parse_service_options(opts, parser, cfg_flag, cfg_opt, cfg_opts);
    REQUIRE(opts.service.start_service);
    REQUIRE(opts.service.service_name == std::string("svc"));
    REQUIRE(opts.service.start_service_name == std::string("svc"));
}

TEST_CASE("parse_service_options missing attach or background names") {
    auto cfg_flag = [](const std::string&) { return false; };
    auto cfg_opt = [](const std::string&) -> std::string { return ""; };
    std::map<std::string, std::string> cfg_opts;

    {
        const char* argv[] = {"prog", "--attach"};
        ArgParser parser(2, const_cast<char**>(argv));
        Options opts;
        REQUIRE_THROWS_AS(parse_service_options(opts, parser, cfg_flag, cfg_opt, cfg_opts),
                          std::runtime_error);
    }

    {
        const char* argv[] = {"prog", "--background"};
        ArgParser parser(2, const_cast<char**>(argv));
        Options opts;
        REQUIRE_THROWS_AS(parse_service_options(opts, parser, cfg_flag, cfg_opt, cfg_opts),
                          std::runtime_error);
    }
}

TEST_CASE("parse_tracker_options config and flags") {
    const char* argv[] = {"prog", "--no-cpu-tracker", "--net-tracker"};
    ArgParser parser(3, const_cast<char**>(argv));
    std::set<std::string> cfg_flags{"--no-mem-tracker"};
    auto cfg_flag = [&](const std::string& f) { return cfg_flags.count(f) > 0; };
    Options opts;
    parse_tracker_options(opts, parser, cfg_flag);
    REQUIRE_FALSE(opts.cpu_tracker);
    REQUIRE_FALSE(opts.mem_tracker);
    REQUIRE(opts.net_tracker);
    REQUIRE(opts.thread_tracker);
}

TEST_CASE("parse_options service flags") {
    const char* argv[] = {"prog", "path",     "--install-service", "--service-config",
                          "cfg",  "--persist"};
    Options opts = parse_options(6, const_cast<char**>(argv));
    REQUIRE(opts.service.install_service);
    REQUIRE(opts.service.service_config == std::string("cfg"));
    REQUIRE(opts.service.persist);
    REQUIRE(opts.service.attach_name == std::string("path"));
    const char* argv2[] = {"prog", "path", "--uninstall-service"};
    Options opts2 = parse_options(3, const_cast<char**>(argv2));
    REQUIRE(opts2.service.uninstall_service);
}

TEST_CASE("parse_options service control flags") {
    const char* argv[] = {"prog", "path", "--start-service", "--stop-service", "--restart-service"};
    Options opts = parse_options(5, const_cast<char**>(argv));
    REQUIRE(opts.service.start_service);
    REQUIRE(opts.service.stop_service);
    REQUIRE(opts.service.restart_service);
    REQUIRE(opts.service.start_service_name == std::string("autogitpull"));
    REQUIRE(opts.service.stop_service_name == std::string("autogitpull"));
    REQUIRE(opts.service.restart_service_name == std::string("autogitpull"));
}

TEST_CASE("parse_options dry run flag") {
    const char* argv[] = {"prog", "path", "--dry-run"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.dry_run);
}

TEST_CASE("parse_options daemon control flags") {
    const char* argv[] = {"prog", "path", "--start-daemon", "--stop-daemon", "--restart-daemon"};
    Options opts = parse_options(5, const_cast<char**>(argv));
    REQUIRE(opts.service.start_daemon);
    REQUIRE(opts.service.stop_daemon);
    REQUIRE(opts.service.restart_daemon);
    REQUIRE(opts.service.start_daemon_name == std::string("autogitpull"));
    REQUIRE(opts.service.stop_daemon_name == std::string("autogitpull"));
    REQUIRE(opts.service.restart_daemon_name == std::string("autogitpull"));
}

TEST_CASE("parse_options service name flags") {
    const char* argv[] = {"prog", "path", "--service-name", "svc"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.service.service_name == std::string("svc"));
}

TEST_CASE("parse_options start service name override") {
    const char* argv[] = {"prog", "path", "--start-service", "svc"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.service.start_service);
    REQUIRE(opts.service.start_service_name == std::string("svc"));
}

TEST_CASE("parse_options start service default name") {
    const char* argv[] = {"prog", "path", "--service-name", "svc", "--start-service"};
    Options opts = parse_options(5, const_cast<char**>(argv));
    REQUIRE(opts.service.start_service);
    REQUIRE(opts.service.start_service_name == std::string("svc"));
}

TEST_CASE("parse_options daemon name flag") {
    const char* argv[] = {"prog", "path", "--daemon-name", "dname"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.service.daemon_name == std::string("dname"));
}

TEST_CASE("parse_options install daemon name override") {
    const char* argv[] = {"prog", "path", "--install-daemon", "dname"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.service.install_daemon);
    REQUIRE(opts.service.daemon_name == std::string("dname"));
}

TEST_CASE("parse_options install service name override") {
    const char* argv[] = {"prog", "path", "--install-service", "svc"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.service.install_service);
    REQUIRE(opts.service.service_name == std::string("svc"));
}

TEST_CASE("parse_options start daemon name override") {
    const char* argv[] = {"prog", "path", "--start-daemon", "dname"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.service.start_daemon);
    REQUIRE(opts.service.start_daemon_name == std::string("dname"));
}

TEST_CASE("parse_options start daemon default name") {
    const char* argv[] = {"prog", "path", "--daemon-name", "dname", "--start-daemon"};
    Options opts = parse_options(5, const_cast<char**>(argv));
    REQUIRE(opts.service.start_daemon);
    REQUIRE(opts.service.start_daemon_name == std::string("dname"));
}

TEST_CASE("parse_options show service flag") {
    const char* argv[] = {"prog", "path", "--show-service"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service.show_service);
}

TEST_CASE("parse_options service status flag") {
    const char* argv[] = {"prog", "path", "--service-status"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service.service_status);
}

TEST_CASE("parse_options daemon status flag") {
    const char* argv[] = {"prog", "path", "--daemon-status"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service.daemon_status);
}

TEST_CASE("parse_options list services flags") {
    const char* argv[] = {"prog", "path", "--list-services"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service.list_services);
    const char* argv2[] = {"prog", "path", "--list-daemons"};
    Options opts2 = parse_options(3, const_cast<char**>(argv2));
    REQUIRE(opts2.service.list_services);
}

TEST_CASE("parse_options attach option") {
    const char* argv[] = {"prog", "--attach", "foo"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service.attach_name == std::string("foo"));
}

TEST_CASE("parse_options background option") {
    const char* argv[] = {"prog", "path", "--background", "foo"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.service.run_background);
    REQUIRE(opts.service.attach_name == std::string("foo"));
}

TEST_CASE("parse_options reattach option") {
    const char* argv[] = {"prog", "--reattach", "foo"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service.reattach);
    REQUIRE(opts.service.attach_name == std::string("foo"));
}

TEST_CASE("parse_options include dir option") {
    const char* argv[] = {"prog", "path", "--include-dir", "a", "--include-dir", "b"};
    Options opts = parse_options(6, const_cast<char**>(argv));
    REQUIRE(opts.root == std::filesystem::path("path"));
    REQUIRE(opts.include_dirs.size() == 2);
    REQUIRE(opts.include_dirs[0] == std::filesystem::path("a"));
    REQUIRE(opts.include_dirs[1] == std::filesystem::path("b"));
}

TEST_CASE("parse_options persist name option") {
    const char* argv[] = {"prog", "path", "--persist=myrun"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service.persist);
    REQUIRE(opts.service.attach_name == std::string("myrun"));
}

TEST_CASE("parse_options runtime options") {
    const char* argv[] = {"prog", "path", "--show-runtime", "--max-runtime", "1h"};
    Options opts = parse_options(5, const_cast<char**>(argv));
    REQUIRE(opts.show_runtime);
    REQUIRE(opts.runtime_limit == std::chrono::hours(1));
}

TEST_CASE("parse_options interval units") {
    const char* argv[] = {"prog", "path", "--interval", "2m"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.interval == 120);
}

TEST_CASE("parse_options kill-all option") {
    const char* argv[] = {"prog", "path", "--kill-all"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service.kill_all);
}

TEST_CASE("parse_options hard reset flags") {
    const char* argv[] = {"prog", "path", "--hard-reset", "--confirm-reset"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.hard_reset);
    REQUIRE(opts.confirm_reset);
}

TEST_CASE("parse_options kill-on-sleep option") {
    const char* argv[] = {"prog", "path", "--kill-on-sleep"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.service.kill_on_sleep);
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

TEST_CASE("parse_options refresh rate units") {
    const char* argv[] = {"prog", "path", "--refresh-rate", "2s"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.refresh_ms == std::chrono::seconds(2));
}

TEST_CASE("parse_options respawn delay units") {
    const char* argv[] = {"prog", "path", "--respawn-delay", "2s"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.service.respawn_delay == std::chrono::seconds(2));
}

TEST_CASE("parse_options poll duration units") {
    const char* argv[] = {"prog",       "path", "--cpu-poll",    "2m",
                          "--mem-poll", "1m",   "--thread-poll", "30s"};
    Options opts = parse_options(8, const_cast<char**>(argv));
    REQUIRE(opts.limits.cpu_poll_sec == 120);
    REQUIRE(opts.limits.mem_poll_sec == 60);
    REQUIRE(opts.limits.thread_poll_sec == 30);
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
    REQUIRE(opts.logging.use_syslog);
    REQUIRE(opts.logging.syslog_facility == 5);
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

TEST_CASE("parse_options tracker flags") {
    const char* argv[] = {"prog", "path", "--no-mem-tracker", "--net-tracker"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE_FALSE(opts.mem_tracker);
    REQUIRE(opts.net_tracker);
}

TEST_CASE("parse_options limit flags") {
    const char* argv[] = {"prog", "path", "--mem-limit", "100MB", "--upload-limit", "1MB"};
    Options opts = parse_options(6, const_cast<char**>(argv));
    REQUIRE(opts.limits.mem_limit == 100);
    REQUIRE(opts.limits.upload_limit == 1024);
}

TEST_CASE("parse_options dont skip timeouts") {
    const char* argv[] = {"prog", "path", "--dont-skip-timeouts"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE_FALSE(opts.limits.skip_timeout);
}

TEST_CASE("parse_options retry skipped") {
    const char* argv[] = {"prog", "path", "--retry-skipped"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.retry_skipped);
}

TEST_CASE("parse_options pull timeout") {
    const char* argv[] = {"prog", "path", "--pull-timeout", "2m"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.limits.pull_timeout == std::chrono::minutes(2));
}

TEST_CASE("parse_options exit on timeout") {
    const char* argv[] = {"prog", "path", "--exit-on-timeout"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.limits.exit_on_timeout);
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
        ofs << "{\n  \"root\": \"/tmp\",\n  \"repositories\": {\n    \"/tmp/repo\": {\n      "
               "\"force-pull\": true,\n      \"exclude\": true,\n      \"check-only\": true,\n     "
               " \"cpu-limit\": 25.5,\n      \"post-pull-hook\": \"/tmp/hook\"\n    }\n  }\n}";
    }
    std::string p = cfg.string();
    char* path_c = strdup(p.c_str());
    char* argv[] = {const_cast<char*>("prog"), const_cast<char*>("--config-json"), path_c};
    Options opts = parse_options(3, argv);
    free(path_c);
    REQUIRE(opts.root == fs::path("/tmp"));
    REQUIRE(opts.repo_settings.count(fs::path("/tmp/repo")) == 1);
    const RepoOptions& ro = opts.repo_settings[fs::path("/tmp/repo")];
    REQUIRE(ro.force_pull.value_or(false));
    REQUIRE(ro.exclude.value_or(false));
    REQUIRE(ro.check_only.value_or(false));
    REQUIRE(ro.cpu_limit.value_or(0.0) == 25.5);
    REQUIRE(ro.post_pull_hook.value() == fs::path("/tmp/hook"));
    fs::remove(cfg);
}

TEST_CASE("parse_options post pull hook flag") {
    const char* argv[] = {"prog", "path", "--post-pull-hook", "/tmp/hook"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.post_pull_hook == fs::path("/tmp/hook"));
}

struct DirGuard {
    std::filesystem::path old;
    explicit DirGuard(const std::filesystem::path& p) : old(std::filesystem::current_path()) {
        std::filesystem::current_path(p);
    }
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
    char* argv[] = {const_cast<char*>(exe.c_str()), const_cast<char*>(root_s.c_str()),
                    const_cast<char*>("--auto-config")};
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
    char* argv[] = {const_cast<char*>(exe.c_str()), const_cast<char*>(root_s.c_str()),
                    const_cast<char*>("--auto-config")};
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
    char* argv[] = {const_cast<char*>(exe.c_str()), const_cast<char*>(root_s.c_str()),
                    const_cast<char*>("--auto-config")};
    Options opts = parse_options(3, argv);
    fs::remove_all(root_dir);
    fs::remove_all(cwd_dir);
    fs::remove_all(exe_dir);
    REQUIRE(opts.interval == 3);
}

TEST_CASE("parse_options wait empty flag") {
    const char* argv[] = {"prog", "path", "--wait-empty"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.wait_empty);
    REQUIRE(opts.wait_empty_limit == 0);
}

TEST_CASE("parse_options wait empty with limit") {
    const char* argv[] = {"prog", "path", "--wait-empty", "5"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.wait_empty);
    REQUIRE(opts.wait_empty_limit == 5);
}

TEST_CASE("parse_options censor names flag") {
    const char* argv[] = {"prog", "path", "--censor-names"};
    Options opts = parse_options(3, const_cast<char**>(argv));
    REQUIRE(opts.censor_names);
    REQUIRE(opts.censor_char == '*');
}

TEST_CASE("parse_options censor char") {
    const char* argv[] = {"prog", "path", "--censor-char", "#"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(!opts.censor_names);
    REQUIRE(opts.censor_char == '#');
}

TEST_CASE("parse_options max log size") {
    const char* argv[] = {"prog", "path", "--max-log-size", "100KB"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.logging.max_log_size == 100 * 1024);
}

TEST_CASE("parse_options alert flags") {
    const char* argv[] = {"prog", "path", "--confirm-alert", "--sudo-su"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.confirm_alert);
    REQUIRE(opts.sudo_su);
}

TEST_CASE("parse_options ssh key paths") {
    const char* argv[] = {"prog", "path", "--ssh-public-key", "pub", "--ssh-private-key", "priv"};
    Options opts = parse_options(6, const_cast<char**>(argv));
    REQUIRE(opts.ssh_public_key == std::filesystem::path("pub"));
    REQUIRE(opts.ssh_private_key == std::filesystem::path("priv"));
}

TEST_CASE("parse_options credential file option") {
    const char* argv[] = {"prog", "path", "--credential-file", "creds.txt"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.credential_file == std::filesystem::path("creds.txt"));
}

TEST_CASE("credential callback selection") {
    Options opts;
    opts.ssh_public_key = "pubkey";
    opts.ssh_private_key = "privkey";
    git_credential* cred = nullptr;
    int r = git::credential_cb(&cred, "url", "git", GIT_CREDENTIAL_SSH_KEY, &opts);
    REQUIRE(r == 0);
    REQUIRE(cred->credtype == GIT_CREDENTIAL_SSH_KEY);
    git_credential_free(cred);

    git_credential* cred2 = nullptr;
    r = git::credential_cb(&cred2, "url", "git", GIT_CREDENTIAL_SSH_KEY | GIT_CREDENTIAL_USERNAME,
                           nullptr);
    REQUIRE(r == 0);
    REQUIRE(cred2->credtype == GIT_CREDENTIAL_USERNAME);
    git_credential_free(cred2);

    setenv("GIT_USERNAME", "user", 1);
    setenv("GIT_PASSWORD", "pass", 1);
    git_credential* cred3 = nullptr;
    r = git::credential_cb(&cred3, "url", nullptr, GIT_CREDENTIAL_USERPASS_PLAINTEXT, nullptr);
    REQUIRE(r == 0);
    REQUIRE(cred3->credtype == GIT_CREDENTIAL_USERPASS_PLAINTEXT);
    git_credential_free(cred3);
}

TEST_CASE("mutant mode requires confirmation") {
    const char* argv[] = {"prog", "--mutant", "--root", "/tmp"};
    REQUIRE_THROWS(parse_options(4, const_cast<char**>(argv)));
    const char* argv2[] = {"prog", "--mutant", "--confirm-mutant", "--root", "/tmp"};
    Options opts = parse_options(5, const_cast<char**>(argv2));
    REQUIRE(opts.mutant_mode);
    REQUIRE(opts.service.run_background);
}

TEST_CASE("credential callback file") {
    fs::path cred = fs::temp_directory_path() / "creds.txt";
    {
        std::ofstream ofs(cred);
        ofs << "user\npass\n";
    }
    Options opts;
    opts.credential_file = cred;
    git_credential* cred_out = nullptr;
    int r = git::credential_cb(&cred_out, "url", nullptr, GIT_CREDENTIAL_USERPASS_PLAINTEXT, &opts);
    REQUIRE(r == 0);
    REQUIRE(cred_out->credtype == GIT_CREDENTIAL_USERPASS_PLAINTEXT);
    auto* up = reinterpret_cast<git_credential_userpass_plaintext*>(cred_out);
    REQUIRE(std::string(up->username) == "user");
    REQUIRE(std::string(up->password) == "pass");
    git_credential_free(cred_out);
    fs::remove(cred);
}

TEST_CASE("parse_options proxy flag") {
    const char* argv[] = {"prog", "/tmp", "--proxy", "http://proxy:8080"};
    Options opts = parse_options(4, const_cast<char**>(argv));
    REQUIRE(opts.proxy_url == std::string("http://proxy:8080"));
}

TEST_CASE("set_proxy applies libgit2 option") {
    git::GitInitGuard guard;
    git::set_proxy("http://proxy:8080");
#ifdef GIT_OPT_GET_PROXY
    const char* proxy = nullptr;
    git_libgit2_opts(GIT_OPT_GET_PROXY, &proxy);
    REQUIRE(proxy != nullptr);
    REQUIRE(std::string(proxy) == "http://proxy:8080");
#endif
    git::set_proxy("");
}
