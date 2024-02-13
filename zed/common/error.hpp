#pragma once

#include "zed/common/util/singleton.hpp"
// C++
#include <expected>
#include <system_error>

namespace zed {

enum class Error : int {
    NullSeq,
    AsyncTimeout,
    InvalidSockAddrs,
};

class ZedCategory : public std::error_category {
public:
    auto name() const noexcept -> const char * override {
        return "zed";
    }
    auto message(int ev) const -> std::string override {
        return error_to_string(static_cast<Error>(ev));
    }

private:
    static constexpr auto error_to_string(Error error) -> const char * {
        switch (error) {
        case Error::NullSeq:
            return "Null io_uring_seq";
        case Error::AsyncTimeout:
            return "Asychronous I/O timeout";
        case Error::InvalidSockAddrs:
            return "Invalid socket addresses";
        default:
            return "Remeber implement error_to_string for new error";
        }
    }
};

auto zed_category() noexcept -> const ZedCategory & {
    return util::Singleton<ZedCategory>::instance();
}

[[nodiscard]]
static inline auto make_zed_error(Error err) -> std::error_code {
    return std::error_code{static_cast<int>(err), zed_category()};
}

[[nodiscard]]
static inline auto make_sys_error(int err) -> std::error_code {
    return std::error_code{err, std::system_category()};
}

template <typename T>
using Result = std::expected<T, std::error_code>;

} // namespace zed

namespace std {

template <>
struct is_error_condition_enum<zed::Error> : public true_type {};

} // namespace std
