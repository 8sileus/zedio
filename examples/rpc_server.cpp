#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

// C
#include <cstdint>

// C++
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "examples/rpc_examples/person.hpp"
#include "examples/rpc_examples/rpc_util.hpp"

using namespace zedio::log;
using namespace zedio::io;
using namespace zedio::net;
using namespace zedio::async;
using namespace zedio::example;
using namespace zedio;

class RpcServer;
auto process(TcpStream stream, RpcServer *server) -> Task<void>;

class RpcServer {
public:
    RpcServer(std::string_view host, uint16_t port)
        : host_{host}
        , port_{port} {}

public:
    template <typename Func>
    void register_handler(std::string_view method, Func fn) {
        map_invokers[{method.data(), method.size()}]
            = [fn]([[maybe_unused]] std::string_view serialized_parameters, std::string &result) {
                  // TODO: parse args
                  auto res = fn();
                  result = serialize(res);
              };
    }

    auto invoke(std::string_view method) -> RpcResult<std::string> {
        console.debug("invoke method={}", method);

        auto it = map_invokers.find({method.data(), method.size()});
        if (it == map_invokers.end()) {
            return std::unexpected{make_rpc_error(RpcError::UnregisteredMethod)};
        } else {
            // TODO: args
            std::string result;
            it->second("", result);
            return result;
        }
    }

    void run() {
        auto runtime = Runtime::create();
        runtime.block_on([this]() -> Task<void> {
            auto has_addr = SocketAddr::parse(host_, port_);
            if (!has_addr) {
                console.error(has_addr.error().message());
                co_return;
            }
            auto has_listener = TcpListener::bind(has_addr.value());
            if (!has_listener) {
                console.error(has_listener.error().message());
                co_return;
            }
            console.info("Listening on {}:{} ...", host_, port_);
            auto listener = std::move(has_listener.value());
            while (true) {
                auto has_stream = co_await listener.accept();

                if (has_stream) {
                    auto &[stream, peer_addr] = has_stream.value();
                    console.info("Accept a connection from {}", peer_addr);
                    spawn(process(std::move(stream), this));
                } else {
                    console.error(has_stream.error().message());
                    break;
                }
            }
        }());
    }

private:
    std::string host_;
    uint16_t    port_;
    std::unordered_map<std::string, std::function<void(std::string_view name, std::string &result)>>
        map_invokers{};
};

auto process(TcpStream stream, RpcServer *server) -> Task<void> {
    RpcFramed rpc_framed{stream};

    while (true) {
        std::array<char, 1024> buf{};

        auto req = co_await rpc_framed.read_frame<RpcMessage>(buf);
        if (!req) {
            console.info("client closed");
            co_return;
        }

        auto method_name = req.value().payload;
        auto has_result = server->invoke(method_name);
        if (!has_result) {
            console.error("{}: {}", has_result.error().message(), method_name);
            co_return;
        }
        RpcMessage resp{has_result.value()};

        auto res = co_await rpc_framed.write_frame<RpcMessage>(resp);
        if (!res) {
            console.error("{}", res.error().message());
            co_return;
        }
    }
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::Debug);
    RpcServer server{"127.0.0.1", 9000};

    server.register_handler("get_int", []() -> int {
        console.debug("get_int() called");
        return 42;
    });

    server.register_handler("get_person", []() -> Person {
        console.debug("get_person() called");
        return {"zhangsan", 18};
    });

    // TODO
    // server.register_handler("add", [](int a, int b) -> int { return a + b; });

    server.run();
}
