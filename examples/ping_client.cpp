#include "zed/async.hpp"
#include "zed/log.hpp"
#include "zed/net.hpp"

using namespace zed::async;
using namespace zed::net;
using namespace zed::log;

auto client(const SocketAddr &addr) -> Task<void> {
    auto        stream = (co_await TcpStream::connect(addr)).value();
    char        buf[5] = {"ping"};
    while(true){
        co_await stream.write_all(buf);
        co_await stream.read(buf);
        console.info("{}", buf);
    }
}

auto main(int argc, char **argv) -> int {
    if (argc != 4) {
        std::cerr << "usage: echo_server ip port client_num\n";
        return -1;
    }
    SET_LOG_LEVEL(zed::log::LogLevel::TRACE);
    auto ip = argv[1];
    auto port = std::stoi(argv[2]);
    auto addr = SocketAddr::parse(ip, port).value();
    auto runtime = Runtime::create();
    auto client_num = std::stoi(argv[3]);
    for (auto i = 0; i < client_num; i += 1) {
        spwan(client(addr));
    }
    runtime.run();
    return 0;
}