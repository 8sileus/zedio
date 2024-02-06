#include "zed/async.hpp"
#include "zed/log.hpp"
#include "zed/net.hpp"

#include <string_view>

using namespace zed::async;
using namespace zed::net;
using namespace zed::log;

constexpr std::string_view response = R"(
HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 13

Hello, World!
)";

auto process(TcpStream stream) -> Task<void> {
    char buf[1024];
    while (true) {
        auto ok = co_await stream.read(buf);
        if (!ok) {
            // console.error(ok.error().message());
            break;
        }
        if (ok.value() == 0) {
            break;
        }
        co_await stream.write(response);
    }
}

auto server() -> Task<void> {
    auto has_addr = SocketAddr::parse("192.168.15.33", 8888);
    if (!has_addr) {
        console.error(has_addr.error().message());
        co_return;
    }
    auto has_listener = TcpListener::bind(has_addr.value());
    if (!has_listener) {
        console.error(has_listener.error().message());
        co_return;
    }
    auto listener = std::move(has_listener.value());
    while (true) {
        // check memory leaking
        // auto has_stream = co_await timeout(listener.accept(), 40s);
        auto has_stream = co_await listener.accept();
        if (has_stream) {
            auto &[stream, peer_addr] = has_stream.value();
            LOG_INFO("Accept a connection from {}", peer_addr.to_string());
            spwan(process(std::move(stream)));
        } else {
            console.error(has_stream.error().message());
            break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: benchmark thread_num\n";
        return -1;
    }
    SET_LOG_LEVEL(zed::log::LogLevel::TRACE);
    auto    thread_num = std::stoi(argv[1]);
    auto    runtime = Runtime::Builder().set_worker_num(thread_num).build();
    runtime.block_on(server());
    return 0;
}