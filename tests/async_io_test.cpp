#include "async.hpp"
#include "common/macros.hpp"
#include "log.hpp"
#include "net/socket_addr.hpp"
#include "net/tcp_listener.hpp"
#include "net/tcp_stream.hpp"
// C
#include <assert.h>
// C++
#include <barrier>
#include <format>

using namespace zed::net;
using namespace zed::log;
using namespace zed::async;

std::barrier finish{3};

auto client(std::barrier<std::__empty_completion> &b, SocketAddr host_addr) -> Task<void> {
    b.arrive_and_wait();
    auto t = co_await TcpStream::connect(host_addr);
    assert(t);
    auto stream = std::move(t.value());
    char buf[64];
    co_await stream.write("hi", sizeof("hi"));
    co_await stream.read(buf, sizeof(buf));
    std::cout << "client: " << buf << "\n";
    char s[] = "8silues";
    co_await stream.write(s, sizeof(s));
    finish.arrive_and_drop();
}

auto server(std::barrier<std::__empty_completion> &b, SocketAddr local_addr) -> Task<void> {
    auto ex = TcpListener::bind(local_addr);
    if (!ex.has_value()) {
        zed::LOG_ERROR("{}", ex.error().message());
        exit(-1);
    }
    auto listener = std::move(ex.value());
    if (auto result = listener.set_reuse_address(true); !result) {
        zed::LOG_ERROR("{}", result.error().message());
    }
    b.arrive_and_drop();
    auto stream = (co_await listener.accept()).value();
    assert(stream.get_fd() >= 0);
    zed::LOG_DEBUG("peer_addr:{}", stream.peer_address().value().to_string());
    char buf[64]{};
    co_await stream.read(buf, sizeof(buf));
    std::cout << "server: " << buf << "\n";
    co_await stream.write("hi", sizeof("hi"));
    co_await stream.read(buf, sizeof(buf));
    std::cout << "server: " << buf << "\n";
    finish.arrive_and_drop();
}

int main() {
    zed::async::detail::Dispatcher dis;
    std::barrier                   b(2);
    auto                           addr = SocketAddr::parse("localhost", 9191).value();
    dis.dispatch(server(b, addr));
    dis.dispatch(client(b, addr));
    finish.arrive_and_wait();
    return 0;
}
