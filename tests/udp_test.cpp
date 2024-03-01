#include "zedio/core.hpp"
#include "zedio/net.hpp"

#include <iostream>

using namespace zedio;
using namespace zedio::net;
using namespace zedio::async;

auto client(std::string_view ip, uint16_t port) -> Task<void> {
    auto             addr = SocketAddr::parse(ip, port).value();
    auto             sock = net::UdpDatagram::v4().value();
    std::string_view w_buf = "udp ping";
    char             r_buf[1024] = {};
    while (true) {
        co_await sock.send_to(w_buf, addr);
        auto [len, addr] = (co_await sock.recv_from(r_buf)).value();
        if (len == 0) {
            break;
        }
        LOG_INFO("read: {} from {}", len, addr);
    }
}

auto server(std::string_view ip, uint16_t port) -> Task<void> {
    for (int i = 0; i < 10; i += 1) {
        spawn(client(ip, port));
    }
    auto addr = SocketAddr::parse(ip, port).value();
    auto sock = net::UdpDatagram::bind(addr).value();
    char buf[1024] = {};
    for (int i = 0; i < 100; i += 1) {
        auto [len, addr] = (co_await sock.recv_from(buf)).value();
        LOG_INFO("read: {} from {}", len, addr);
        co_await sock.send_to({buf, len}, addr);
    }
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::TRACE);
    auto ip = "127.0.0.1";
    auto port = 9999;
    Runtime::create().block_on(server(ip, static_cast<uint16_t>(port)));
    return 0;
}