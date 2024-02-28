#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class SendTo : public detail::IORegistrator<SendTo, decltype(io_uring_prep_sendto)> {
private:
    using Super = detail::IORegistrator<SendTo, decltype(io_uring_prep_sendto)>;

public:
    SendTo(int                    sockfd,
           const void            *buf,
           size_t                 len,
           int                    flags,
           const struct sockaddr *addr,
           socklen_t              addrlen)
        : Super{io_uring_prep_sendto, sockfd, buf, len, flags, addr, addrlen} {}

    template <typename Addr>
    SendTo(int sockfd, std::span<const char> buf, int flags, const Addr &addr)
        : Super{io_uring_prep_sendto,
                sockfd,
                buf.data(),
                buf.size_bytes(),
                flags,
                addr.sockaddr(),
                addr.length()} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return static_cast<std::size_t>(this->cb_.result_);
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
