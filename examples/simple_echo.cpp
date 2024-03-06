#include "zedio/core.hpp"
#include "zedio/fs.hpp"
#include "zedio/net.hpp"

using namespace zedio;
using namespace zedio::async;
using namespace zedio::net;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024]{};
    while (true) {
        auto len = (co_await (stream.read(buf))).value();
        if (len == 0) {
            break;
        }
        LOG_INFO("{}", std::string_view{buf, len});
        co_await stream.write_all({buf, len});
    }
}

auto server() -> Task<void> {
    auto addr = SocketAddr::parse("localhost", 9999).value();
    auto listener = TcpListener::bind(addr).value();

    while (true) {
        auto ret = co_await listener.accept();
        if (ret) {
            auto &[stream, addr] = ret.value();
            LOG_INFO("{}", addr);
            spawn(process(std::move(stream)));
        } else {
            LOG_ERROR("{}", ret.error());
        }
    }
}

auto main() -> int {
    Runtime::create().block_on(server());
}