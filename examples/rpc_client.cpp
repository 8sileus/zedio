#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

// C
#include <cstdint>

// C++
#include <sstream>
#include <string_view>
#include <utility>

#include "examples/rpc_examples/person.hpp"
#include "examples/rpc_examples/rpc_util.hpp"

using namespace zedio::log;
using namespace zedio::io;
using namespace zedio::net;
using namespace zedio::async;
using namespace zedio::example;
using namespace zedio;

class RpcClient {
private:
    explicit RpcClient(TcpStream &&stream)
        : stream_{std::move(stream)} {}

public:
    static auto connect(std::string_view host, uint16_t port) -> Task<RpcResult<RpcClient>> {
        auto addr = SocketAddr::parse(host, port).value();
        auto stream = co_await TcpStream::connect(addr);
        if (!stream) {
            console.error("{}", stream.error().message());
            co_return std::unexpected{make_rpc_error(RpcError::ConnectFailed)};
        }
        co_return RpcClient{std::move(stream.value())};
    }

    template <typename Ret, typename... Args>
    auto call(std::string_view method_name, Args &&...args) -> Task<RpcResult<Ret>> {
        RpcFramed              rpc_framed{stream_};
        std::array<char, 1024> buf{};

        std::ostringstream oss;
        oss << method_name << ' ';
        ((oss << args << ' '), ...);
        auto payload = oss.str();
        if (!payload.empty()) {
            payload.pop_back();
        }

        RpcMessage req{payload};
        auto       res = co_await rpc_framed.write_frame<RpcMessage>(req);
        if (!res) {
            console.error("{}", res.error().message());
            co_return std::unexpected{make_rpc_error(RpcError::WriteFrameFailed)};
        }

        auto resp = co_await rpc_framed.read_frame<RpcMessage>(buf);
        if (!resp) {
            console.error("{}", resp.error().message());
            co_return std::unexpected{make_rpc_error(RpcError::ReadFrameFailed)};
        }

        auto data = resp.value().payload;
        co_return deserialize<Ret>(data);
    }

private:
    TcpStream stream_;
};

auto client() -> Task<void> {
    auto res = co_await RpcClient::connect("127.0.0.1", 9000);
    if (!res) {
        console.error("{}", res.error().message());
        co_return;
    }
    auto client = std::move(res.value());

    auto person = (co_await client.call<Person>("get_person")).value();
    console.info("get_person name={}, age={}", person.name, person.age);

    auto add_result = (co_await client.call<int>("add", 1, 2)).value();
    console.info("add(1, 2) result={}", add_result);

    auto mul_result = (co_await client.call<double>("mul", 64, 64)).value();
    console.info("mul(64, 64) result={}", mul_result);

    // auto call_result = co_await client.call<float>("xxxx");
    // if (!call_result) {
    //     console.error("{}", call_result.error().message());
    //     co_return;
    // } else {
    //     console.info("xxxx {}", call_result.value());
    // }
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::Debug);
    auto runtime = Runtime::options().scheduler().set_num_workers(1).build();
    runtime.block_on(client());
}
