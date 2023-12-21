#include "async.hpp"
#include "log.hpp"
#include "net.hpp"

#include <string_view>

using namespace zed::async;
using namespace zed::net;
using namespace zed::log;

std::string_view response = R"(
HTTP/1.1 200 OK
Content-Length: 88
Content-Type: text/html

<html>
<body>
<h1>It Works!</h1>
</body>
</html>
                            )";

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
        LOG_TRACE("{}", std::string_view{buf, static_cast<std::size_t>(ok.value())});
        ok = co_await stream.write(response.data(), response.size());
        if (!ok) {
            console.error(ok.error().message());
            break;
        }
        LOG_DEBUG("process {} end", stream.get_fd());
    }
}

Task<void> accept() {
    auto has_addr = SocketAddr::parse("192.168.15.33", 8888);
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
        auto has_stream = co_await listener.accept();
        if (has_stream) {
            console.info("Accept a connection from {}",
                         has_stream.value().peer_address().value().to_string());
            spawn(process(std::move(has_stream.value())));
        } else {
            console.error(has_stream.error().message());
            break;
        }
    }
}

int main() {
    SET_LOG_LEVEL(zed::log::LogLevel::TRACE);
    Runtime runtime;
    spawn(accept());
    runtime.run();
}