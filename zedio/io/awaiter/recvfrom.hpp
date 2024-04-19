#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

// TODOOODOOOOO wait io_uring_prep_recvfrom

namespace detail {

    class RecvFrom : public IORegistrator<RecvFrom> {
    private:
        using Super = IORegistrator<RecvFrom>;

    public:
        RecvFrom(int              sockfd,
                 void            *buf,
                 size_t           len,
                 int              flags,
                 struct sockaddr *addr,
                 socklen_t       *addrlen)
            : Super{io_uring_prep_recvmsg, sockfd, &msg_, flags}
            , iovec_{.iov_base = buf, .iov_len = len}
            , msg_{.msg_name = addr,
                   .msg_namelen = addrlen != nullptr ? *addrlen : 0,
                   .msg_iov = &iovec_,
                   .msg_iovlen = len,
                   .msg_control = nullptr,
                   .msg_controllen = 0,
                   .msg_flags = flags}
            , addrlen_{addrlen} {}

        auto await_resume() const noexcept -> Result<std::size_t> {
            if (addrlen_) {
                *addrlen_ = msg_.msg_namelen;
            }
            if (this->cb_.result_ >= 0) [[likely]] {
                return static_cast<std::size_t>(this->cb_.result_);
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }

    private:
        struct iovec  iovec_;
        struct msghdr msg_;
        socklen_t    *addrlen_;
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto
recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen) {
    return detail::RecvFrom{sockfd, buf, len, flags, addr, addrlen};
}

} // namespace zedio::io
