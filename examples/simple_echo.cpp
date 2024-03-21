#include "zedio/core.hpp"
#include "zedio/net.hpp"
#include "zedio/time.hpp"

using namespace zedio;
using namespace zedio::async;
using namespace zedio::net;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024]{};
    while (true) {
        auto ret = co_await stream.read(buf);
        if (!ret) {
            LOG_ERROR("{}", ret.error());
            break;
        } 
        auto len = ret.value();
        LOG_DEBUG("{}", std::string_view(buf, len));
        if (len == 0) {
            break;
        }
        co_await stream.write_all({buf, len});
    }
}

auto server() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 9999).value();
    auto listener = TcpListener::bind(addr).value();
    while (true) {
        auto [stream, addr] = (co_await listener.accept()).value();
        LOG_INFO("{}", addr);
        spawn(process(std::move(stream)));
    }
}

auto main() -> int {
    Runtime::create().block_on(server());
}