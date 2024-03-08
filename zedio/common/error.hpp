#pragma once

#include "zedio/common/util/singleton.hpp"
// C++
#include <expected>
#include <string_view>
// C
#include <cassert>
#include <cstring>
#include <format>

namespace zedio {

class Error {
public:
    enum {
        EmptySqe = 8000,
        InvalidAddresses,
        ClosedChannel,
    };

    explicit Error(int err_code)
        : err_code_{err_code} {}

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
        default:
            return strerror(err_code_);
        }
    }

private:
    int err_code_;
};

[[nodiscard]]
static inline auto make_zedio_error(int err) -> Error {
    assert(err >= 0);
    return Error{err};
}

[[nodiscard]]
static inline auto make_sys_error(int err) -> Error {
    assert(err >= 0);
    return Error{err};
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