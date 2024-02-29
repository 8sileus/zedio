#include "zedio/core.hpp"
// C++
#include <iostream>
// Linux
#include <arpa/inet.h>

using namespace zedio;
using namespace zedio::async;
using namespace zedio::io;

auto client(std::string_view ip, uint16_t port, int num_connections) -> Task<void> {
    if (num_connections > 0) {
        spawn(client(ip, port, num_connections - 1));
    }
    auto ret = co_await Socket(AF_INET, SOCK_STREAM, 0, 0);
    if (!ret) {
        LOG_ERROR("socket failed, {}", ret.error().message());
    }
    auto               fd = ret.value();
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.data(), &addr.sin_addr);
    addr.sin_port = ::htons(port);

    if (auto ret
        = co_await Connect(fd, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));
        ret) {

        std::string_view w_buf= "ping";
        char             r_buf[1024];
        while (true) {
            if (auto ret = co_await Send{fd, w_buf.data(), w_buf.size(), MSG_NOSIGNAL}; ret) {
                ret = co_await Recv(fd, r_buf, sizeof(r_buf), 0);
                if (!ret) {
                    LOG_ERROR("recv failed, {}", ret.error().message());
                    break;
                }
                LOG_INFO("{}", std::string_view{r_buf, ret.value()});
            } else {
                LOG_ERROR("send failed, {}", ret.error().message());
                break;
            }
        }
    } else {
        LOG_ERROR("connect failed, {}", ret.error().message());
    }

    if (auto ret = co_await Close(fd); !ret) {
        LOG_ERROR("close failed, {}", ret.error().message());
    }
}

auto main(int argc, char **argv) -> int {
    if (argc != 5) {
        std::cerr << "usage: unwrap_client ip port num_threads num_connections\n";
        return -1;
    }
    SET_LOG_LEVEL(zedio::log::LogLevel::TRACE);
    auto ip = argv[1];
    auto port = std::stoi(argv[2]);
    auto num_threads = std::stoi(argv[3]);
    auto num_connections = std::stoi(argv[4]);
    Runtime::options()
        .set_num_workers(num_threads)
        .build()
        .block_on(client(ip, static_cast<uint16_t>(port), num_connections));
    return 0;
}