#pragma once

#include "zedio/common/util/singleton.hpp"
// C++
#include <expected>
#include <string_view>
// C
#include <cassert>
#include <cstring>
#include <format>

#ifdef _WIN32
// Win
#include <WinSock2.h>
#endif

namespace zedio {

class Error {
public:
    enum ErrorCode {
        EmptySqe = 8000,
        InvalidAddresses,
        ClosedChannel,
        UnexpectedEOF,
        WriteZero,
        TooLongTime,
        PassedTime,
        InvalidSocketType,
        ReuniteFailed,
    };

public:
    explicit Error(int err_code)
        : err_code_{err_code} {}

public:
    [[nodiscard]]
    auto value() const noexcept -> int {
        return err_code_;
    }

    [[nodiscard]]
    auto message() const noexcept -> std::string_view {
        switch (err_code_) {
        case EmptySqe:
            return "No sqe is available";
        case InvalidAddresses:
            return "Invalid addresses";
        case ClosedChannel:
            return "Channel has closed";
        case UnexpectedEOF:
            return "Read EOF too early";
        case WriteZero:
            return "Write return zero";
        case TooLongTime:
            return "Time is too long";
        case PassedTime:
            return "Time has passed";
        case InvalidSocketType:
            return "Invalid socket type";
        case ReuniteFailed:
            return "Tried to reunite halves that are not from the same socket";
        default:
            return strerror(err_code_);
        }
    }

private:
    int err_code_;
};

[[nodiscard]]
static inline auto make_error(int err) -> Error {
    assert(err >= 0);
    return Error{err};
}

[[nodiscard]]
static inline auto make_system_error() -> Error {
#ifdef __linux__
    return Error{errno};
#elif _WIN32
    return Error{static_cast<int>(GetLastError())};
#endif
}

[[nodiscard]]
static inline auto make_socket_error() -> Error {
#ifdef __linux__
    return Error{errno};
#elif _WIN32
    return Error{static_cast<int>(WSAGetLastError())};
#endif
}

template <typename T>
using Result = std::expected<T, Error>;

} // namespace zedio

namespace std {

template <>
class formatter<zedio::Error> {
public:
    constexpr auto parse(format_parse_context &context) {
        auto it{context.begin()};
        auto end{context.end()};
        if (it == end || *it == '}') {
            return it;
        }
        ++it;
        if (it != end && *it != '}') {
            throw format_error("Invalid format specifier for Error");
        }
        return it;
    }

    auto format(const zedio::Error &error, auto &context) const noexcept {
        return format_to(context.out(), "{} (error {})", error.message(), error.value());
    }
};

} // namespace std