#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Connect : public IORegistrator<Connect> {
    private:
        using Super = IORegistrator<Connect>;

    public:
        Connect(int fd, const struct sockaddr *addr, socklen_t addrlen)
            : Super{io_uring_prep_connect, fd, addr, addrlen} {}

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
auto connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    return detail::Connect{fd, addr, addrlen};
}

} // namespace zedio::io
