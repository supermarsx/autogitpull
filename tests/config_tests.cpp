#include "test_common.hpp"

TEST_CASE("YAML config loading") {
    fs::path cfg = fs::temp_directory_path() / "cfg.yaml";
    {
        std::ofstream ofs(cfg);
        ofs << "interval: 42\n";
        ofs << "cli: true\n";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_yaml_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--interval"] == "42");
    REQUIRE(opts["--cli"] == "true");
    fs::remove(cfg);
}

#if 0
TEST_CASE("YAML config categories") {
    fs::path cfg = fs::temp_directory_path() / "cfg_cat.yaml";
    {
        std::ofstream ofs(cfg);
        ofs << "General:\n  interval: 10\n  cli: true\nLogging:\n  log-level: DEBUG\n";
    }
    std::map<std::string, std::string> opts;
    std::string err;
    REQUIRE(load_yaml_config(cfg.string(), opts, err));
    REQUIRE(opts["--interval"] == "10");
    REQUIRE(opts["--cli"] == "true");
    REQUIRE(opts["--log-level"] == "DEBUG");
    fs::remove(cfg);
}
#endif

TEST_CASE("JSON config categories") {
    fs::path cfg = fs::temp_directory_path() / "cfg_cat.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"General\": {\n    \"interval\": 10,\n    \"cli\": true\n  },\n  "
               "\"Logging\": {\n    \"log-level\": \"DEBUG\"\n  }\n}";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_json_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--interval"] == "10");
    REQUIRE(opts["--cli"] == "true");
    REQUIRE(opts["--log-level"] == "DEBUG");
    fs::remove(cfg);
}

TEST_CASE("JSON config loading") {
    fs::path cfg = fs::temp_directory_path() / "cfg.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"interval\": 42, \n  \"cli\": true\n}";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_json_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--interval"] == "42");
    REQUIRE(opts["--cli"] == "true");
    fs::remove(cfg);
}

TEST_CASE("YAML config root option") {
    fs::path cfg = fs::temp_directory_path() / "cfg_root.yaml";
    {
        std::ofstream ofs(cfg);
        ofs << "root: /tmp/repos\n";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_yaml_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--root"] == "/tmp/repos");
    fs::remove(cfg);
}

TEST_CASE("JSON config root option") {
    fs::path cfg = fs::temp_directory_path() / "cfg_root.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"root\": \"/tmp/repos\"\n}";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_json_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--root"] == "/tmp/repos");
    fs::remove(cfg);
}

TEST_CASE("JSON repositories section") {
    fs::path cfg = fs::temp_directory_path() / "cfg_repo.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"repositories\": {\n    \"/tmp/repo\": {\n      \"force-pull\": true,\n      \"upload-limit\": 50\n    }\n  }\n}";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_json_config(cfg.string(), opts, repo, err));
    REQUIRE(repo.count("/tmp/repo") == 1);
    REQUIRE(repo["/tmp/repo"]["--force-pull"] == "true");
    REQUIRE(repo["/tmp/repo"]["--upload-limit"] == "50");
    fs::remove(cfg);
}

