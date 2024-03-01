#include "zedio/core.hpp"
#include "zedio/net.hpp"

using namespace zedio::async;
using namespace zedio::net;
using namespace zedio;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024];
    while (true) {
        auto len = (co_await stream.read(buf)).value();
        if (len == 0) {
            break;
        }
        co_await stream.write_all({buf, len});
    }
}

auto server() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 9898).value();
    auto listener = TcpListener::bind(addr).value();
    while (true) {
        auto [stream, peer_addr] = (co_await listener.accept()).value();
        spawn(process(std::move(stream)));
    }
}

auto main() -> int {
    Runtime::create().block_on(server());
    return 0;
}