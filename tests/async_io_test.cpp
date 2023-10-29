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
    auto              t = co_await TcpStream::connect(host_addr);
    assert(t);
    auto              stream = std::move(t.value());
    std::vector<char> buf;
    for (auto c : std::string("8sileus")) {
        buf.push_back(c);
    }
    co_await stream.write(buf);
    stream.close();
}

auto server(std::promise<void> &p, Address local_addr) -> Task<void> {
    auto t = TcpListener::bind(local_addr);
    assert(t);
    auto listener = t.value();
    listener.set_reuse_address(true);
    p.set_value();
    auto s = co_await listener.accept();
    assert(s);
    auto              stream = std::move(s.value());
    std::vector<char> buf(64);
    co_await stream.read(buf);
    std::cout << buf.data() << "\n";
    stream.close();
}

int main() {
    Scheduler          s;
    std::promise<void> p;
    s.stop(1s);
    Address addr("localhost", "9898");
    s.submit(server(p, addr));
    s.submit(client(p, addr));
    s.start();
    return 0;
}
