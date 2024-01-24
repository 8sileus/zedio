#include "zed/async.hpp"
#include "zed/log.hpp"
#include "zed/net.hpp"

using namespace zed::async;
using namespace zed::net;
using namespace zed::log;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024];
    // char buf1[5];
    // char buf2[1024];
    while (true) {
        // auto ok = co_await stream.read_vectored(buf1, buf2);
        auto ok = co_await stream.read(buf);
        // error or peer close connection
        if (!ok) {
            console.error(ok.error().message());
            break;
        }
        if (ok.value() == 0) {
            break;
        }

        auto len = ok.value();
        // console.info("read: {}", std::string_view{buf, static_cast<std::size_t>(len)});

        // ok = co_await stream.write_vectored(buf1, buf2);
        ok = co_await stream.write(buf, len);
        if (!ok) {
            console.error(ok.error().message());
            break;
        }
    }
}

Task<void> t1() {
    LOG_DEBUG("1");
    co_return;
}
Task<void> t2() {
    LOG_DEBUG("2");
    co_return;
}
Task<void> t3() {
    LOG_DEBUG("3");
    // check exception
    //  LOG_DEBUG("{} {}", 3);
    co_return;
}

auto server() -> Task<void> {
    spwan(t1(), t2(), t3());
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
        // auto has_stream = co_await timeout(listener.accept(), 3s);
        auto has_stream = co_await listener.accept();
        if (has_stream) {
            auto [stream, peer_addr] = std::move(has_stream.value());
            console.info("Accept a connection from {}", peer_addr.to_string());
            spwan(process(std::move(stream)));
        } else {
            console.error(has_stream.error().message());
            continue;
        }
    }
}

int main() {
    SET_LOG_LEVEL(zed::log::LogLevel::TRACE);
    Runtime runtime;
    runtime.block_on(server());
    return 0;
}