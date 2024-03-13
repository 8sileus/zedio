#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

using namespace zedio::async;
using namespace zedio::net;
using namespace zedio;
using namespace zedio::log;

auto client(const UnixSocketAddr &addr) -> Task<void> {
    auto ret = co_await UnixStream::connect(addr);
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

auto server(const UnixSocketAddr &addr) -> Task<void> {
    auto ret = UnixListener::bind(addr);
    if (!ret) {
        console.error("{}", ret.error().message());
        co_return;
    }
    auto listener = std::move(ret.value());
    spawn(client(addr));
    auto [stream, peer_addr] = (co_await listener.accept()).value();
    console.info("server: {}", listener.local_addr().value().pathname());
    console.info("client: {}", peer_addr.pathname());
    std::string_view str = "tcp_test pong";
    char             buf[1024];
    for (int i = 0; i <= 10; i += 1) {
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
    auto addr = UnixSocketAddr::parse("tmp_test");
    if (!addr) {
        co_return;
    }
    co_await server(addr.value());
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::TRACE);
    return Runtime::options().scheduler().set_num_workers(1).build().block_on(test());
}