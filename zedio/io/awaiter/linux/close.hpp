#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Close : public IORegistrator<Close> {
    private:
        using Super = IORegistrator<Close>;

    public:
        Close(int fd)
            : Super{io_uring_prep_close, fd} {}

        auto await_resume() const noexcept -> Result<void> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return {};
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto close(int fd) {
    return detail::Close{fd};
}

} // namespace zedio::io
