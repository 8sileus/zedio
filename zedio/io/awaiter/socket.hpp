#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Socket : public detail::IORegistrator<Socket> {
    private:
        using Super = detail::IORegistrator<Socket>;

    public:
        Socket(int domain, int type, int protocol, unsigned int flags)
            : Super{io_uring_prep_socket, domain, type, protocol, flags} {}

        auto await_resume() const noexcept -> Result<int> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return this->cb_.result_;
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto socket(int domain, int type, int protocol, unsigned int flags) {
    return detail::Socket{domain, type, protocol, flags};
}

} // namespace zedio::io
