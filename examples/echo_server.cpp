#include "zed/async.hpp"
#include "zed/log.hpp"
#include "zed/net.hpp"

using namespace zed::async;
using namespace zed::net;
using namespace zed::log;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024];
    while (true) {
        auto ok = co_await timeout(stream.read(buf, sizeof(buf)), 5s);
        // error or peer close connection
        if (!ok) {
            console.error(ok.error().message());
            break;
        }
        if (ok.value() == 0) {
            break;
        }

        auto len = ok.value();
        console.info("read: {}", std::string_view{buf, static_cast<std::size_t>(len)});

        ok = co_await stream.write(buf, len);
        if (!ok) {
            console.error(ok.error().message());
            break;
        }
    }
    LOG_DEBUG("process {} end", stream.get_fd());
}

auto accept_handle() -> Task<void> {
    auto has_addr = SocketAddr::parse("localhost", 9898);
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
    auto _ = listener.set_reuse_address(true);
    while (true) {
        auto has_stream = co_await timeout(listener.accept(), 3s);
        if (has_stream) {
            console.info("Accept a connection from {}",
                         has_stream.value().peer_address().value().to_string());
            spwan(process(std::move(has_stream.value())));
        } else {
            console.error(has_stream.error().message());
            continue;
        }
    }
}

int main(int args, char **argv) {
    SET_LOG_LEVEL(zed::log::LogLevel::TRACE);
    Runtime runtime;
    runtime.block_on(accept_handle());
}