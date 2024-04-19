#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

// C
#include <cstdint>

// C++
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "examples/rpc_examples/function_traits.hpp"
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
        map_invokers_[{method.data(), method.size()}] = [fn](std::string_view serialized_parameters,
                                                             std::string     &result) {
            // Get function args type
            using args_tuple_type = typename function_traits<Func>::args_tuple_type;
            args_tuple_type args;

            std::istringstream iss({serialized_parameters.data(), serialized_parameters.size()});
            std::apply([&iss](auto &...arg) { ((iss >> arg), ...); }, args);

            auto res = std::apply(fn, args);
            result = serialize(res);
        };
    }

    // Payload format:
    // +--------+--------+--------+--------+--------+
    // | method |  arg1  |  arg2  |  arg3  | ...    |
    // +--------+--------+--------+--------+--------+
    // Spaces are used as delimiters
    auto invoke(std::string_view payload) -> RpcResult<std::string> {
        std::istringstream iss{
            {payload.data(), payload.size()}
        };
        std::string method;
        iss >> method;
        console.debug("invoke method={}", method);
        auto it = map_invokers_.find({method.data(), method.size()});
        if (it == map_invokers_.end()) {
            return std::unexpected{make_rpc_error(RpcError::UnregisteredMethod)};
        } else {
            std::string parameters;
            std::string result;
            // FIXME
            std::getline(iss, parameters);
            it->second(parameters, result);
            return result;
        }
    }

    void run() {
        auto runtime = zedio::runtime::Builder<>::default_create();
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
                    spawn(this->handle_rpc(std::move(stream)));
                } else {
                    console.error(has_stream.error().message());
                    break;
                }
            }
        }());
    }

private:
    auto handle_rpc(TcpStream stream) -> Task<void> {
        RpcFramed rpc_framed{stream};

        while (true) {
            std::array<char, 1024> buf{};

            auto req = co_await rpc_framed.read_frame<RpcMessage>(buf);
            if (!req) {
                console.info("client closed");
                co_return;
            }

            auto payload = req.value().payload;
            auto has_result = this->invoke(payload);
            if (!has_result) {
                console.error("{}: {}", has_result.error().message(), payload);
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

private:
    std::string host_;
    uint16_t    port_;
    std::unordered_map<std::string, std::function<void(std::string_view name, std::string &result)>>
        map_invokers_;
};

auto add(int a, int b) -> int {
    return a + b;
}

auto mul(double a, double b) -> double {
    return a * b;
}

auto get_person() -> Person {
    return {"zhangsan", 18};
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::Debug);
    RpcServer server{"127.0.0.1", 9000};

    server.register_handler("add", add);
    server.register_handler("mul", mul);
    server.register_handler("get_person", get_person);

    server.run();
}
