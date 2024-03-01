#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Fsync : public detail::IORegistrator<Fsync> {
private:
    using Super = detail::IORegistrator<Fsync>;

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

} // namespace zedio::io
