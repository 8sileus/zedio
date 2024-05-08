#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

using namespace zedio::async;
using namespace zedio::net;
using namespace zedio::log;
using namespace zedio;

auto process(TcpStream stream) -> Task<void> {
    char buf[1024];
    // char buf1[5];
    // char buf2[1024];
    while (true) {
        // auto ok = co_await stream.read_vectored(buf1, buf2);
        auto ok = co_await stream.read(buf);
        // error or peer close connection
        if (!ok) {
            console.error("{} {}", ok.error().value(), ok.error().message());
            break;
        }
        if (ok.value() == 0) {
            break;
        }

        auto len = ok.value();
        buf[len] = 0;
        console.info("read: {}", buf);

        // ok = co_await stream.write_vectored(buf1, buf2);
        ok = co_await stream.write({buf, len});
        if (!ok) {
            console.error(ok.error().message());
            break;
        }
    }
}

auto server() -> Task<void> {
    auto has_addr = SocketAddr::parse("127.0.0.1", 9898);
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
    while (true) {
        // auto has_stream = co_await listener.accept().set_timeout(3s).set_exclusion();
        auto has_stream = co_await listener.accept();

        if (has_stream) {
            auto &[stream, peer_addr] = has_stream.value();
            console.info("Accept a connection from {}", peer_addr);
            spawn(process(std::move(stream)));
        } else {
            console.error(has_stream.error().message());
            break;
        }
    }
}

auto main(int argc, char **argv) -> int {
    if (argc != 2) {
        std::cerr << "usage: echo_server num_threas\n";
        return -1;
    }
    SET_LOG_LEVEL(zedio::log::LogLevel::Trace);
    auto num_threas = std::stoi(argv[1]);
    auto runtime
        = zedio::runtime::MultiThreadBuilder::options().set_num_workers(num_threas).build();
    runtime.block_on(server());
    return 0;
}