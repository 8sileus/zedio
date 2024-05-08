#include "zedio/core.hpp"
// C++
#include <iostream>
// Linux
#include <arpa/inet.h>

using namespace zedio;
using namespace zedio::async;

auto process(int fd) -> Task<void> {
    char buf[1024];
    while (true) {
        if (auto ret = co_await io::recv(fd, buf, sizeof(buf), 0); ret && ret.value() > 0) {
            ret = co_await io::send(fd, buf, ret.value(), MSG_NOSIGNAL);
            if (!ret) {
                LOG_ERROR("send failed, {}", ret.error().message());
                break;
            }
            LOG_INFO("{}", std::string_view{buf, ret.value()});
        } else {
            if (!ret) {
                LOG_ERROR("recv failed, {}", ret.error().message());
            }
            break;
        }
    }
    if (auto ret = co_await io::close(fd); !ret) {
        LOG_ERROR("close failed, {}", ret.error().message());
    }
}

auto server(std::string_view ip, uint16_t port) -> Task<void> {
    auto ret = co_await io::socket(AF_INET, SOCK_STREAM, 0, 0);
    if (!ret) {
        LOG_ERROR("socket failed, {}", ret.error().message());
    }
    auto               fd = ret.value();
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.data(), &addr.sin_addr);
    addr.sin_port = ::htons(port);
    ::bind(fd, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));
    ::listen(fd, SOMAXCONN);

    struct sockaddr_in peer_addr {};
    socklen_t          addrlen{};
    while (true) {
        auto ret
            = co_await io::accept(fd, reinterpret_cast<struct sockaddr *>(&peer_addr), &addrlen, 0);
        if (!ret) {
            LOG_ERROR("accept failed, {}", ret.error().message());
        }
        spawn(process(ret.value()));
    }

    if (auto ret = co_await io::close(fd); !ret) {
        LOG_ERROR("close failed, {}", ret.error().message());
    }
}

auto main(int argc, char **argv) -> int {
    if (argc != 4) {
        std::cerr << "usage: unwrap_server ip port num_threads\n";
        return -1;
    }
    SET_LOG_LEVEL(zedio::log::LogLevel::Trace);
    auto ip = argv[1];
    auto port = std::stoi(argv[2]);
    auto num_threads = std::stoi(argv[3]);
    zedio::runtime::MultiThreadBuilder::options()
        .set_num_workers(num_threads)
        .build()
        .block_on(server(ip, static_cast<uint16_t>(port)));
    return 0;
}