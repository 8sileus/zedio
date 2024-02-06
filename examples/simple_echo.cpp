#include "zed/async.hpp"
#include "zed/net.hpp"

using namespace zed::async;
using namespace zed::net;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024];
    while (true) {
        std::size_t len = (co_await stream.read(buf)).value();
        if (len == 0) {
            break;
        }
        co_await stream.write({buf, len});
    }
}

auto server() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 9898).value();
    auto listener = TcpListener::bind(addr).value();
    while (true) {
        auto [stream, peer_addr] = (co_await listener.accept()).value();
        spwan(process(std::move(stream)));
    }
}

auto main() -> int {
    auto runtime = Runtime::Builder().build();
    runtime.block_on(server());
    return 0;
}