#include "async.hpp"
#include "common/macros.hpp"
#include "log.hpp"
#include "net/address.hpp"
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
    auto fd = co_await Socket(host_addr.family(), SOCK_STREAM, 0);
    assert(fd >= 0);
    int ret = co_await Connect(fd, host_addr.get_sockaddr(), host_addr.get_length());
    assert(ret == 0);
    auto msg = "hello world!";
    co_await Write(fd, msg, strlen(msg));
    console.info("[client] write: {}", msg);
    co_await Close(fd);
    co_return;
}

auto server(std::promise<void> &p, Address local_addr) -> Task<void> {
    console.info("[server] local_addr: {}", local_addr.to_string());
    auto fd = ::socket(local_addr.family(), SOCK_STREAM, 0);
    assert(fd >= 0);
    int ret = co_await Bind(fd, local_addr.get_sockaddr(), local_addr.get_length());
    assert(ret == 0);
    ret = co_await Listen(fd, 0);
    assert(ret == 0);
    p.set_value();
    sockaddr addr;
    std::memset(&addr, 0, sizeof(addr));
    socklen_t len;
    int       peer_fd = co_await Accept(fd, &addr, &len, 0);
    assert(peer_fd != -1);
    Address peer_addr(&addr, len);
    console.info("[server] peer_addr: {}", peer_addr.to_string());
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    if (auto n = co_await Read(peer_fd, buf, sizeof(buf)); n < 0) {
        console.info("[server] read failed {}", strerror(n));
        co_return;
    }
    console.info("[server] read: {}", buf);
    co_await Close(peer_fd);
    co_await Close(fd);
    co_return;
}

int main() {
    Scheduler          s;
    std::promise<void> p;
    s.stop(1s);
    Address addr("localhost", "9898");
    s.submit(server(p, addr));
    s.submit(client(p, addr));
    s.start();
    console.info("SUCC");
    return 0;
}
