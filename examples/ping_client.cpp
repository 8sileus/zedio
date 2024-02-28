#include "zedio/async.hpp"
#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

using namespace zedio::async;
using namespace zedio::net;
using namespace zedio::log;
using namespace zedio;

auto process(const SocketAddr &addr) -> Task<void> {
    auto stream = (co_await TcpStream::connect(addr)).value();
    char buf[5] = {"ping"};
    while (true) {
        co_await stream.write_all(buf);
        co_await stream.read(buf);
        console.info("{}", buf);
    }
}

auto client(const SocketAddr &addr, int client_num) -> Task<void> {
    for (auto i = 0; i < client_num; i += 1) {
        spawn(process(addr));
    }
    while (true) {
    }
}

auto main(int argc, char **argv) -> int {
    if (argc != 4) {
        std::cerr << "usage: echo_server ip port client_num\n";
        return -1;
    }
    SET_LOG_LEVEL(zedio::log::LogLevel::TRACE);
    auto ip = argv[1];
    auto port = std::stoi(argv[2]);
    auto addr = SocketAddr::parse(ip, port).value();
    auto runtime = Runtime::create();
    auto client_num = std::stoi(argv[3]);
    runtime.block_on(client(addr, client_num));
    return 0;
}