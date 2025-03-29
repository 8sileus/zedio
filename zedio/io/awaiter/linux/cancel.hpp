#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {
namespace detail {

    class Cancel : public IORegistrator<Cancel> {
    private:
        using Super = IORegistrator<Cancel>;

    public:
        Cancel(int fd, unsigned int flags)
            : Super{io_uring_prep_cancel_fd, fd, flags} {}

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
static inline auto cancel(int fd, unsigned int flags) {
    return detail::Cancel{fd, flags};
}

} // namespace zedio::io
