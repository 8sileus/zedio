#pragma once

#include "zedio/core.hpp"
#include "zedio/log.hpp"
#include "zedio/net.hpp"

// C
#include <cstdint>

// C++
#include <array>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

// Linux
#include <arpa/inet.h>
#include <netinet/in.h>

namespace zedio::example {

using namespace zedio::async;
using namespace zedio::net;
using namespace zedio::log;

static constexpr size_t FIXED_LEN{4uz};

auto read_u32(std::span<char> bytes) -> uint32_t {
    // or ntohl()
    uint32_t value = 0;
    value |= static_cast<uint32_t>(bytes[0]) << 24;
    value |= static_cast<uint32_t>(bytes[1]) << 16;
    value |= static_cast<uint32_t>(bytes[2]) << 8;
    value |= static_cast<uint32_t>(bytes[3]);
    return value;
}

void write_u32(uint32_t value, std::span<uint8_t> bytes) {
    // or htonl()
    bytes[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    bytes[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    bytes[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    bytes[3] = static_cast<uint8_t>(value & 0xFF);
}

template <typename T>
    requires requires(std::string_view buf) {
        { T::deserialize(buf) } -> std::convertible_to<T>;
    }
static auto deserialize(std::string_view data) -> T {
    return T::deserialize({data.data(), data.size()});
}

template <typename T>
    requires std::is_fundamental_v<T>
static auto deserialize(std::string_view data) -> T {
    std::istringstream iss({data.begin(), data.size()});

    T t;
    iss >> t;
    return t;
}

template <typename T>
    requires requires(T t) {
        { t.serialize() } -> std::convertible_to<std::string_view>;
    }
static auto serialize(T t) -> std::string {
    return t.serialize();
}

template <typename T>
    requires std::is_fundamental_v<T>
static auto serialize(T t) -> std::string {
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

struct RpcMessage {
    std::string_view payload;

    void write_to(std::string &buf) const {
        std::array<unsigned char, 4> bytes{};
        uint32_t                     length = payload.size();

        write_u32(length, bytes);
        buf.append(std::string_view{reinterpret_cast<char *>(bytes.data()), bytes.size()});
        buf.append(payload);
    }
};

class RpcError {
public:
    enum ErrorCode {
        InsufficientData = 9000,
        ConnectFailed,
        ReadEOF,
        ReadFailed,
        WriteFailed,
        ReadFrameFailed,
        WriteFrameFailed,
        UnregisteredMethod,
    };

public:
    explicit RpcError(int err_code)
        : err_code_{err_code} {}

public:
    [[nodiscard]]
    auto value() const noexcept -> int {
        return err_code_;
    }

    [[nodiscard]]
    auto message() const noexcept -> std::string_view {
        switch (err_code_) {
        case InsufficientData:
            return "Insufficient data";
        case ConnectFailed:
            return "Connect failed";
        case ReadEOF:
            return "Read eof";
        case ReadFailed:
            return "Read failed";
        case WriteFailed:
            return "Write failed";
        case ReadFrameFailed:
            return "Read frame failed";
        case WriteFrameFailed:
            return "Write frame failed";
        case UnregisteredMethod:
            return "Unregistered method";
        default:
            return strerror(err_code_);
        }
    }

private:
    int err_code_;
};

[[nodiscard]]
static inline auto make_rpc_error(int err) -> RpcError {
    assert(err >= 9000);
    return RpcError{err};
}

template <typename T>
using RpcResult = std::expected<T, RpcError>;

template <typename MessageType>
class RpcCodec {
public:
    auto encode(MessageType &message) -> std::string {
        std::string buf;
        message.write_to(buf);
        return buf;
    }
    auto decode(std::span<char> buf) -> RpcResult<MessageType> {
        if (buf.size() < FIXED_LEN) {
            return std::unexpected{make_rpc_error(RpcError::InsufficientData)};
        }
        auto length = read_u32(buf);
        if (buf.size() - FIXED_LEN < length) {
            return std::unexpected{make_rpc_error(RpcError::InsufficientData)};
        }

        return MessageType{
            std::string_view{buf.begin() + FIXED_LEN, buf.begin() + FIXED_LEN + length}
        };
    }
};

template <typename Codec>
class Framed {
public:
    explicit Framed(TcpStream &stream)
        : stream_{stream} {}

public:
    template <typename FrameType>
    auto read_frame(std::span<char> buf) -> Task<RpcResult<FrameType>> {
        std::size_t nbytes = 0;
        while (true) {
            auto s = buf.subspan(nbytes, buf.size() - nbytes);
            auto read_result = co_await stream_.read(buf);
            if (!read_result) {
                console.error("{}", read_result.error().message());
                co_return std::unexpected{make_rpc_error(RpcError::ReadFailed)};
            }
            if (read_result.value() == 0) {
                co_return std::unexpected{make_rpc_error(RpcError::ReadEOF)};
            }
            nbytes += read_result.value();
            // 解码数据
            auto readed = buf.subspan(0, nbytes);
            auto decode_result = codec_.decode(readed);
            if (!decode_result) {
                if (decode_result.error().value() == RpcError::InsufficientData) {
                    continue;
                } else {
                    console.error("{}", decode_result.error().message());
                    break;
                }
            }
            co_return decode_result;
        }
    }

    template <typename FrameType>
        requires requires(FrameType msg, std::string &buf) { msg.write_to(buf); }
    auto write_frame(FrameType &message) -> Task<RpcResult<bool>> {
        auto encoded = codec_.encode(message);

        auto write_result = co_await stream_.write(encoded);
        if (!write_result) {
            console.error("{}", write_result.error().message());
            co_return std::unexpected{make_rpc_error(RpcError::WriteFailed)};
        }
        co_return true;
    }

private:
    Codec      codec_;
    TcpStream &stream_;
};

using RpcFramed = Framed<RpcCodec<RpcMessage>>;

} // namespace zedio::example
