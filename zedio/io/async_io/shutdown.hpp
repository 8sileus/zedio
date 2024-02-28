#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Shutdown : public detail::IORegistrator<Shutdown, decltype(io_uring_prep_shutdown)> {
private:
    using Super = detail::IORegistrator<Shutdown, decltype(io_uring_prep_shutdown)>;

public:
    enum How : int {
        READ = SHUT_RD,
        WRITE = SHUT_WR,
        RDWR = SHUT_RDWR,
    };

    Shutdown(int fd, int how)
        : Super{io_uring_prep_shutdown, fd, how} {}

    Shutdown(int fd, How how)
        : Super{io_uring_prep_shutdown, fd, static_cast<int>(how)} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
