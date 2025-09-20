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
    FS_REMOVE(cfg);
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
    FS_REMOVE(cfg);
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
    FS_REMOVE(cfg);
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
    FS_REMOVE(cfg);
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
    FS_REMOVE(cfg);
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
    FS_REMOVE(cfg);
}

TEST_CASE("YAML credential file option") {
    fs::path cfg = fs::temp_directory_path() / "cfg_cred.yaml";
    {
        std::ofstream ofs(cfg);
        ofs << "credential-file: creds.txt\n";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_yaml_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--credential-file"] == "creds.txt");
    FS_REMOVE(cfg);
}

TEST_CASE("JSON credential file option") {
    fs::path cfg = fs::temp_directory_path() / "cfg_cred.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"credential-file\": \"creds.txt\"\n}";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_json_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--credential-file"] == "creds.txt");
    FS_REMOVE(cfg);
}

TEST_CASE("JSON repositories section") {
    fs::path cfg = fs::temp_directory_path() / "cfg_repo.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"repositories\": {\n    \"/tmp/repo\": {\n      \"force-pull\": true,\n      "
               "\"upload-limit\": 50\n    }\n  }\n}";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_json_config(cfg.string(), opts, repo, err));
    REQUIRE(repo.count("/tmp/repo") == 1);
    REQUIRE(repo["/tmp/repo"]["--force-pull"] == "true");
    REQUIRE(repo["/tmp/repo"]["--upload-limit"] == "50");
    FS_REMOVE(cfg);
}

TEST_CASE("YAML value conversions") {
    fs::path cfg = fs::temp_directory_path() / "cfg_values.yaml";
    {
        std::ofstream ofs(cfg);
        ofs << "bool_true: true\n";
        ofs << "bool_false: false\n";
        ofs << "int_val: 7\n";
        ofs << "float_val: 3.5\n";
        ofs << "null_val: null\n";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_yaml_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--bool_true"] == "true");
    REQUIRE(opts["--bool_false"] == "false");
    REQUIRE(opts["--int_val"] == "7");
    REQUIRE(opts["--float_val"] == "3.5");
    REQUIRE(opts["--null_val"] == "");
    FS_REMOVE(cfg);
}

TEST_CASE("JSON value conversions") {
    fs::path cfg = fs::temp_directory_path() / "cfg_values.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"bool_true\": true,\n  \"bool_false\": false,\n  \"int_val\": 7,\n  "
               "\"float_val\": 3.5,\n  \"null_val\": null\n}";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_json_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--bool_true"] == "true");
    REQUIRE(opts["--bool_false"] == "false");
    REQUIRE(opts["--int_val"] == "7");
    REQUIRE(opts["--float_val"] == "3.5");
    REQUIRE(opts["--null_val"] == "");
    FS_REMOVE(cfg);
}

TEST_CASE("YAML unknown key allowed") {
    fs::path cfg = fs::temp_directory_path() / "cfg_unknown.yaml";
    {
        std::ofstream ofs(cfg);
        ofs << "unknown: 1\n";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_yaml_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--unknown"] == "1");
    FS_REMOVE(cfg);
}

TEST_CASE("JSON unknown key allowed") {
    fs::path cfg = fs::temp_directory_path() / "cfg_unknown.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"unknown\": 1\n}";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE(load_json_config(cfg.string(), opts, repo, err));
    REQUIRE(opts["--unknown"] == "1");
    FS_REMOVE(cfg);
}

TEST_CASE("YAML type mismatch is reported") {
    fs::path cfg = fs::temp_directory_path() / "cfg_type.yaml";
    {
        std::ofstream ofs(cfg);
        ofs << "interval:\n  sub: 1\n";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE_FALSE(load_yaml_config(cfg.string(), opts, repo, err));
    REQUIRE(err.find("interval") != std::string::npos);
    FS_REMOVE(cfg);
}

TEST_CASE("JSON type mismatch is reported") {
    fs::path cfg = fs::temp_directory_path() / "cfg_type.json";
    {
        std::ofstream ofs(cfg);
        ofs << "{\n  \"interval\": {\n    \"sub\": 1\n  }\n}";
    }
    std::map<std::string, std::string> opts;
    std::map<std::string, std::map<std::string, std::string>> repo;
    std::string err;
    REQUIRE_FALSE(load_json_config(cfg.string(), opts, repo, err));
    REQUIRE(err.find("interval") != std::string::npos);
    FS_REMOVE(cfg);
}
