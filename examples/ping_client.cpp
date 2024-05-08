#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

using namespace zedio::async;
using namespace zedio::net;
using namespace zedio::log;
using namespace zedio;

auto client(const SocketAddr &addr, int client_num) -> Task<void> {
    if (client_num > 0) {
        spawn(client(addr, client_num - 1));
    }
    auto ret = co_await TcpStream::connect(addr);
    if (!ret) {
        LOG_ERROR("{}", ret.error().message());
        co_return;
    }
    auto &stream = ret.value();
    char  w_buf[5] = {"ping"};
    char  r_buf[64] = {};
    while (true) {
        co_await stream.write_all(w_buf);
        auto ret = co_await stream.read(r_buf);
        if (!ret || ret.value() == 0) {
            break;
        }
        console.info("{}", r_buf);
    }
}

auto main(int argc, char **argv) -> int {
    if (argc != 4) {
        std::cerr << "usage: ping_client ip port num_connection\n";
        return -1;
    }
    SET_LOG_LEVEL(zedio::log::LogLevel::Trace);
    auto ip = argv[1];
    auto port = std::stoi(argv[2]);
    auto addr = SocketAddr::parse(ip, port).value();
    auto client_num = std::stoi(argv[3]);
    zedio::runtime::MultiThreadBuilder::default_create().block_on(client(addr, client_num));
    return 0;
}