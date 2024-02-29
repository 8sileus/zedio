#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

using namespace zedio::async;
using namespace zedio::net;
using namespace zedio::log;
using namespace zedio;

auto client(const SocketAddr &addr) -> Task<void> {
    auto ret = co_await TcpStream::connect(addr);
    if (!ret) {
        console.error("{}", ret.error().message());
        co_return;
    }
    std::string_view str = "tcp_test ping";
    char             buf[1024];
    auto             stream = std::move(ret.value());
    while (true) {
        auto ret = co_await stream.write(str);
        if (!ret) {
            console.error("{}", ret.error().message());
            break;
        }
        ret = co_await stream.read(buf);
        if (!ret || ret.value() == 0) {
            break;
        }
        console.info("client read {}", buf);
    }
}

auto server(const SocketAddr &addr) -> Task<void> {
    auto ret = TcpListener::bind(addr);
    if (!ret) {
        console.error("{}", ret.error().message());
        co_return;
    }
    auto listener = std::move(ret.value());
    spawn(client(addr));
    auto [stream, peer_addr] = (co_await listener.accept()).value();
    console.info("{}", peer_addr.to_string());
    std::string_view str = "tcp_test pong";
    char             buf[1024];
    while (true) {
        auto ret = co_await stream.read(buf);
        if (!ret || ret.value() == 0) {
            break;
        }
        console.info("server read {}", buf);
        ret = co_await stream.write(str);
        if (!ret) {
            console.error("{}", ret.error().message());
            break;
        }
    }
}

auto test() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 9999);
    if (!addr) {
        console.error("{}", addr.error().message());
        co_return;
    }
    spawn(server(addr.value()));
    co_await zedio::io::Sleep(10s);
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::TRACE);
    auto runtime = Runtime::options().set_num_workers(1).build();
    runtime.block_on(test());
    return 0;
}