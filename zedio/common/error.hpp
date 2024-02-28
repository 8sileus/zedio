#pragma once

#include "zedio/common/util/singleton.hpp"
// C++
#include <expected>
#include <system_error>

namespace zedio {

enum class Error : int {
    NoError = 0,
    AsyncTimeout,
    InvalidInput,
    InvalidOutput,
};

class ZedioCategory : public std::error_category {
public:
    auto name() const noexcept -> const char * override {
        return "zedio";
    }

    auto message(int ev) const noexcept -> std::string override {
        return error_to_string(static_cast<Error>(ev));
    }

    // auto equivalent(int i, const std::error_condition &cond) const noexcept -> bool override;

    auto equivalent(const std::error_code &code, int cond) const noexcept -> bool {
        return std::addressof(code.category()) == this && code.value() == cond;
    }

private:
    static constexpr auto error_to_string(Error error) -> const char * {
        switch (error) {
        case Error::NoError:
            return "Successful";
        case Error::AsyncTimeout:
            return "Asychronous I/O timeout";
        case Error::InvalidInput:
            return "Invalid input";
        case Error::InvalidOutput:
            return "Invalid Output";
        default:
            return "Remeber implement error_to_string for new error";
        }
    }
};

[[nodiscard]]
static inline auto make_zedio_error(Error err) -> std::error_code {
    return std::error_code{static_cast<int>(err), util::Singleton<ZedioCategory>::instance()};
}

[[nodiscard]]
static inline auto make_sys_error(int err) -> std::error_code {
    return std::error_code{err, std::system_category()};
}

template <typename T>
using Result = std::expected<T, std::error_code>;

} // namespace zedio

namespace std {

template <>
struct is_error_condition_enum<zedio::Error> : public true_type {};

} // namespace std
