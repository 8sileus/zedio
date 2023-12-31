#pragma once

#include "zed/common/util/singleton.hpp"
// C++
#include <system_error>

namespace zed {

enum class Error : int {
    Nosqe,
    IOtimeout,
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
        case Error::Nosqe:
            return "No sqe is available";
        case Error::IOtimeout:
            return "I/O operation timeout";
        default:
            return "Remeber implement error_to_string for new error";
        }
    }
};

auto zed_category() noexcept -> const ZedCategory & {
    return util::Singleton<ZedCategory>::Instance();
}

} // namespace zed

namespace std {
template <>
struct is_error_condition_enum<zed::Error> : public true_type {};

} // namespace std
