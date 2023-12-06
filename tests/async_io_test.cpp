#include "async.hpp"
#include "common/macros.hpp"
#include "log.hpp"
#include "net/address.hpp"
#include "net/tcp_listener.hpp"
#include "net/tcp_stream.hpp"
// C
#include <assert.h>
// C++
#include <format>
#include <future>

using namespace zed::net;
using namespace zed::log;
using namespace zed::async;

auto client(std::promise<void> &p, Address host_addr) -> Task<void> {
    p.get_future().get();
    auto t = co_await TcpStream::connect(host_addr);
    assert(t);
    auto              stream = std::move(t.value());
    char              buf[64];
    co_await stream.write("hi", sizeof("hi"));
    co_await stream.read(buf, sizeof(buf));
    std::cout << "client: " << buf << "\n";
    char s[] = "8silues";
    co_await stream.write(s, sizeof(s));
}

auto server(std::promise<void> &p, Address local_addr) -> Task<void> {
    auto t = TcpListener::bind(local_addr);
    assert(t);
    auto listener = std::move(t.value());
    listener.set_reuse_address(true);
    p.set_value();
    auto fd = co_await listener.accept();
    assert(fd >= 0);
    auto              stream = TcpStream{fd};
    zed::LOG_DEBUG("peer_addr:{}", stream.get_peer_address().to_string());
    char buf[64]{};
    co_await stream.read(buf, sizeof(buf));
    std::cout << "server: " << buf << "\n";
    co_await stream.write("hi", sizeof("hi"));
    co_await stream.read(buf, sizeof(buf));
    std::cout << "server: " << buf << "\n";
}

int main() {
    Scheduler          s;
    std::promise<void> p;
    s.stop(1s);
    Address addr("localhost", "9191");
    s.submit(server(p, addr));
    s.submit(client(p, addr));
    s.start();
    return 0;
}
