#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class SendTo : public IORegistrator<SendTo> {
    private:
        using Super = IORegistrator<SendTo>;

    public:
        SendTo(int                    sockfd,
               const void            *buf,
               size_t                 len,
               int                    flags,
               const struct sockaddr *addr,
               socklen_t              addrlen)
            : Super{io_uring_prep_sendto, sockfd, buf, len, flags, addr, addrlen} {}

        auto await_resume() const noexcept -> Result<std::size_t> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return static_cast<std::size_t>(this->cb_.result_);
            } else {
                return std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto sendto(int                    sockfd,
                          const void            *buf,
                          size_t                 len,
                          int                    flags,
                          const struct sockaddr *addr,
                          socklen_t              addrlen) {
    return detail::SendTo{sockfd, buf, len, flags, addr, addrlen};
}

} // namespace zedio::io
