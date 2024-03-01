#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Accept : public detail::IORegistrator<Accept> {
private:
    using Super = detail::IORegistrator<Accept>;

public:
    Accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags)
        : Super{io_uring_prep_accept, fd, addr, addrlen, flags} {}

    auto await_resume() const noexcept -> Result<int> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return this->cb_.result_;
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
