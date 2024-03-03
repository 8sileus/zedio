#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Fsync : public IORegistrator<Fsync> {
    private:
        using Super = IORegistrator<Fsync>;

    public:
        Fsync(int fd, unsigned fsync_flags)
            : Super{io_uring_prep_fsync, fd, fsync_flags} {}

        auto await_resume() const noexcept -> Result<void> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return {};
            } else {
                return std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
auto fsync(int fd, unsigned fsync_flags) {
    return detail::Fsync{fd, fsync_flags};
}

} // namespace zedio::io
