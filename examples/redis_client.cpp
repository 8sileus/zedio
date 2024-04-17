// ----------------------------------------------
// https://redis.io/docs/reference/protocol-spec/
//
// # Simple strings
//
// Simple strings are encoded as a plus (+) character, followed by a string.
// The string mustn't contain a CR (\r) or LF (\n) character and is terminated by CRLF (i.e., \r\n).
//
// examples:
//   "OK" -> +OK\r\n  (5bytes)
// ----------------------------------------------
// # Bulk strings
//
// RESP encodes bulk strings in the following way:
//
// $<length>\r\n<data>\r\n
//
//   - The dollar sign ($) as the first byte.
//   - One or more decimal digits (0..9) as the string's length, in bytes, as an unsigned, base-10
//   value.
//   - The CRLF terminator.
//   - The data.
//   - A final CRLF.
//
// examples:
//   "hello" -> $5\r\nhello\r\n (11bytes)
//   ""      -> $0\r\n\r\n      (6bytes)
//   null    -> $-1\r\n         (5bytes) # RESP3 dedicated data type for null values
// ----------------------------------------------
// # Arrays
//
// Clients send commands to the Redis server as RESP arrays. Similarly, some Redis commands that
// return collections of elements use arrays as their replies. An example is the LRANGE command that
// returns elements of a list.
//
// RESP Arrays' encoding uses the following format:
//
// *<number-of-elements>\r\n<element-1>...<element-n>
//
//   - An asterisk (*) as the first byte.
//   - One or more decimal digits (0..9) as the number of elements in the array as an unsigned,
//   base-10 value.
//   - The CRLF terminator.
//   - An additional RESP type for every element of the array.
//
// examples:
//   GET hello       -> *2\r\n$3\r\nGET\r\n$5\r\nhello\r\n
//   SET hello world -> *3\r\n$3\r\nSET\r\n$5\r\nhello\r\n$5\r\nworld\r\n
// ----------------------------------------------

#include "zedio/core.hpp"
#include "zedio/io/buf/reader.hpp"
#include "zedio/io/buf/writer.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"
#include "zedio/socket/split.hpp"
#include "zedio/socket/stream.hpp"

// C
#include <cassert>
// C++
#include <format>
#include <string>
#include <string_view>
#include <utility>

using namespace zedio::log;
using namespace zedio::io;
using namespace zedio::net;
using namespace zedio::async;
using namespace zedio;

using OwnedReader = socket::detail::OwnedReadHalf<TcpStream, SocketAddr>;
using OwnedWriter = socket::detail::OwnedWriteHalf<TcpStream, SocketAddr>;
using BufferedReader = BufReader<OwnedReader>;
using BufferedWriter = BufWriter<OwnedWriter>;

class RedisProtocolCodec {
    enum class MsgType {
        Simple,
        Bulk,
        Unknown,
    };

public:
    auto decode(BufferedReader &reader) -> Task<std::string> {
        char ch{};
        // read 1 byte
        auto ret = co_await reader.read_exact({&ch, 1});
        if (!ret) {
            console.error("decode error: {}", ret.error().message());
            co_return std::string{};
        }
        console.trace("ch: {}", ch);

        MsgType type{MsgType::Unknown};
        switch (ch) {
        case '+':
            type = MsgType::Simple;
            break;
        case '$':
            type = MsgType::Bulk;
            break;
        default:
            std::unreachable();
        }

        if (type == MsgType::Simple) {
            std::string buf;
            co_await reader.read_until(buf, "\r\n");
            co_return std::string{ch + buf};
        } else {
            assert(type == MsgType::Bulk);
            std::string buf_len;
            co_await reader.read_until(buf_len, "\r\n");
            auto len = std::stol(buf_len);

            // Null bulk strings
            if (len == -1) {
                co_return std::string{ch + buf_len};
            }

            std::string buf_msg;
            co_await reader.read_until(buf_msg, "\r\n");
            assert(len == static_cast<int>(buf_msg.size()) - 2);
            co_return std::format("{}{}{}", ch, buf_len, buf_msg);
        }
    }

    auto encode(const std::span<const char> message, BufferedWriter &writer) -> Task<void> {
        auto ret = co_await writer.write_all(message);
        if (!ret) {
            console.error("encode error: {}", ret.error().message());
            co_return;
        }
        co_await writer.flush();
    }
};

class Protocol {
public:
    explicit Protocol(TcpStream &&stream) {
        auto [reader, writer] = stream.into_split();
        buffered_reader_ = BufReader(std::move(reader));
        buffered_writer_ = BufWriter(std::move(writer));
    }

public:
    auto send_set_command(std::string_view key, std::string_view value) -> Task<void> {
        auto command = std::format("*3\r\n$3\r\nSET\r\n${}\r\n{}\r\n${}\r\n{}\r\n",
                                   std::to_string(key.size()),
                                   key,
                                   std::to_string(value.size()),
                                   value);
        console.debug("Send:\n{}", command);
        co_await codec_.encode(command, buffered_writer_);
    }

    auto send_get_command(std::string_view key) -> Task<void> {
        auto command
            = std::format("*2\r\n$3\r\nGET\r\n${}\r\n{}\r\n", std::to_string(key.size()), key);
        console.debug("Send:\n{}", command);
        co_await codec_.encode(command, buffered_writer_);
    }

    auto get_command_result() -> Task<std::string> {
        auto res = co_await codec_.decode(buffered_reader_);
        console.debug("Recv:\n{}", res);
        co_return res;
    }

    auto close() -> Task<void> {
        co_await buffered_reader_.inner().reunite(buffered_writer_.inner()).value().close();
    }

private:
    BufferedReader     buffered_reader_{OwnedReader{nullptr}};
    BufferedWriter     buffered_writer_{OwnedWriter{nullptr}};
    RedisProtocolCodec codec_;
};

class RedisClient {
private:
    explicit RedisClient(TcpStream &&stream)
        : proto_{std::move(stream)} {}

public:
    [[REMEMBER_CO_AWAIT]]
    static auto connect(std::string_view host, uint16_t port) -> Task<Result<RedisClient>> {
        auto addr = SocketAddr::parse(host, port).value();
        auto stream = co_await TcpStream::connect(addr);
        if (!stream) {
            console.error("{}", stream.error().message());
            co_return std::unexpected{make_sys_error(errno)};
        }
        co_return RedisClient{std::move(stream.value())};
    }

    [[REMEMBER_CO_AWAIT]]
    auto set(std::string_view key, std::string_view value) -> Task<void> {
        co_await proto_.send_set_command(key, value);
        auto response = co_await proto_.get_command_result();
        if (response != "+OK\r\n") {
            console.error("SET command failed: {}", response);
        }
    }

    [[REMEMBER_CO_AWAIT]]
    auto get(std::string_view key) -> Task<std::string> {
        co_await proto_.send_get_command(key);
        auto response = co_await proto_.get_command_result();
        if (response.size() < 2 || response.substr(0, 1) != "$") {
            console.error("Invalid response to GET command: {}", response);
            co_return std::string{};
        }
        auto end = response.find('\n');
        if (end == std::string::npos) {
            co_return std::string{};
        } else {
            auto payload_len = std::stoul(response.substr(1, end + 1));
            co_return response.substr(end + 1, end + 1 + payload_len);
        }
    }

    [[REMEMBER_CO_AWAIT]]
    auto close() -> Task<void> {
        co_await proto_.close();
    }

private:
    Protocol proto_;
};

auto client() -> Task<void> {
    auto res = co_await RedisClient::connect("127.0.0.1", 6379);
    if (!res) {
        console.error("{}", res.error().message());
        co_return;
    }
    auto client = std::move(res.value());

    co_await client.set("hello", "world");

    auto result = co_await client.get("hello");
    console.info("client.get(\"hello\"): {}", result);

    // result = co_await client.get("unknown");
    // console.info("client.get(\"unknown\"): {}", result);
    co_await client.close();
}

auto main() -> int {
    SET_LOG_LEVEL(zedio::log::LogLevel::Debug);
    auto runtime = Runtime::options().scheduler().set_num_workers(1).build();
    runtime.block_on(client());
}
