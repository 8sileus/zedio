#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Connect : public detail::IORegistrator<Connect> {
private:
    using Super = detail::IORegistrator<Connect>;

public:
    Connect(int fd, const struct sockaddr *addr, socklen_t addrlen)
        : Super{io_uring_prep_connect, fd, addr, addrlen} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
