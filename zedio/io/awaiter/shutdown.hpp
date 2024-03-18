#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

enum class ShutdownBehavior {
    Read = SHUT_RD,
    Write = SHUT_WR,
    ReadWrite = SHUT_RDWR,
};

namespace detail {

    class Shutdown : public IORegistrator<Shutdown> {
    private:
        using Super = IORegistrator<Shutdown>;

    public:
        Shutdown(int fd, int how)
            : Super{io_uring_prep_shutdown, fd, how} {}

        Shutdown(int fd, ShutdownBehavior how)
            : Super{io_uring_prep_shutdown, fd, static_cast<int>(how)} {}

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
static inline auto shutdown(int fd, int how) {
    return detail::Shutdown{fd, how};
}

} // namespace zedio::io
