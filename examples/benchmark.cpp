#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

#include <string_view>

using namespace zedio::async;
using namespace zedio::net;
using namespace zedio::log;
using namespace zedio;

constexpr std::string_view response = R"(
HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 13

Hello, World!
)";

auto process(TcpStream stream) -> Task<void> {
    char buf[128];
    while (true) {
        if (auto ret = co_await stream.read(buf); !ret || ret.value() == 0) {
            break;
        }
        if (auto ret = co_await stream.write_all(response); !ret) {
            break;
        }
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
        auto has_stream = co_await listener.accept();
        if (has_stream) {
            auto &[stream, peer_addr] = has_stream.value();
            LOG_INFO("Accept a connection from {}", peer_addr);
            spawn(process(std::move(stream)));
        } else {
            console.error(has_stream.error().message());
            break;
        }
    }
}

auto main(int argc, char **argv) -> int {
    if (argc != 2) {
        std::cerr << "usage: benchmark thread_num\n";
        return -1;
    }
    SET_LOG_LEVEL(zedio::log::LogLevel::TRACE);
    auto thread_num = std::stoi(argv[1]);
    auto runtime = Runtime::options().set_num_worker(thread_num).build();
    runtime.block_on(server());
    return 0;
}