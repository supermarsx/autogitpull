#include "test_common.hpp"
#include "webhook_notifier.hpp"
#include <atomic>
#include <thread>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

static std::string received_body;

static uint16_t start_server(std::atomic<bool>& done) {
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(srv, 1);
    socklen_t len = sizeof(addr);
    getsockname(srv, reinterpret_cast<sockaddr*>(&addr), &len);
    uint16_t port = ntohs(addr.sin_port);
    std::thread([srv, &done]() {
        int cli = accept(srv, nullptr, nullptr);
        char buf[1024];
        ssize_t n = read(cli, buf, sizeof(buf));
        if (n > 0) {
            std::string req(buf, n);
            auto pos = req.find("\r\n\r\n");
            if (pos != std::string::npos)
                received_body = req.substr(pos + 4);
        }
        std::string resp = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
        write(cli, resp.c_str(), resp.size());
        close(cli);
        close(srv);
        done = true;
    }).detach();
    return port;
}

TEST_CASE("webhook notifier posts payload") {
    std::atomic<bool> done{false};
    uint16_t port = start_server(done);
    WebhookNotifier notifier("http://127.0.0.1:" + std::to_string(port));
    RepoInfo info;
    info.path = "/tmp/repo";
    info.status = RS_PULL_OK;
    info.message = "ok";
    info.pulled = true;
    notifier.notify(info);
    for (int i = 0; i < 50 && !done; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(done);
    auto j = json::parse(received_body);
    REQUIRE(j["repository"] == info.path.string());
    REQUIRE(j["status"] == "Pulled");
}
