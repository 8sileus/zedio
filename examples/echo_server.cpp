#include "async.hpp"
#include "log.hpp"
#include "net.hpp"

using namespace zed::async;
using namespace zed::net;
using namespace zed::log;

Task<void> process(TcpStream stream) {
    char buf[1024];
    while (true) {
        auto ok = co_await stream.read(buf, sizeof(buf));
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

Task<void> accept() {
    auto has_addr = SocketAddr::parse("localhost", 9999);
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
    // while (true) {
        auto has_stream = co_await listener.accept();
        if (has_stream) {
            console.info("Accept a connection from {}",
                         has_stream.value().peer_address().value().to_string());
            spawn(process(std::move(has_stream.value())));
        } else {
            console.error(has_stream.error().message());
            // break;
        }
    // }
}

int main() {
    SET_LOG_LEVEL(zed::log::LogLevel::TRACE);
    Runtime runtime;
    runtime.block_on(accept());
}