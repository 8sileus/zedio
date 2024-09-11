#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"
#include "zedio/sync.hpp"
#include "zedio/time.hpp"

using namespace zedio::async;
using namespace zedio::net;
using namespace zedio::log;
using namespace zedio::sync;
using namespace zedio;

auto client(const SocketAddr &addr, Latch &latch) -> Task<void> {
    auto ret = co_await TcpStream::connect(addr);
    if (!ret) {
        console.error("{}", ret.error().message());
        co_return;
    }
    std::string_view str = "tcp_test ping";
    char             buf[1024];
    auto             stream = std::move(ret.value());
    auto [reader, writer] = stream.into_split();
    for (auto i{0}; i < 1000; i += 1) {
        auto ret = co_await writer.write(str);
        if (!ret) {
            console.error("{}", ret.error().message());
            break;
        }
        ret = co_await reader.read(buf);
        if (!ret || ret.value() == 0) {
            break;
        }
        console.info("client read {}", buf);
    }
    co_await reader.reunite(writer).value().close();
    co_await latch.arrive_and_wait();
}

auto server(const SocketAddr &addr, Latch &latch) -> Task<void> {
    auto ret = TcpListener::bind(addr);
    if (!ret) {
        console.error("{}", ret.error().message());
        co_return;
    }
    auto listener = std::move(ret.value());
    spawn(client(addr, latch));
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
    co_await latch.arrive_and_wait();
}

auto test() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 9999);
    if (!addr) {
        console.error("{}", addr.error().message());
        co_return;
    }
    Latch latch{3};
    spawn(server(addr.value(), latch));
    co_await latch.arrive_and_wait();
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::Trace);
    runtime::MultiThreadBuilder::default_create().block_on(test());
    return 0;
}