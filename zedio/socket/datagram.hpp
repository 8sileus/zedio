#pragma once

#include "zedio/io/io.hpp"
#include "zedio/socket/impl/impl_datagram_recv.hpp"
#include "zedio/socket/impl/impl_datagram_send.hpp"
#include "zedio/socket/impl/impl_sockopt.hpp"

namespace zedio::socket::detail {

template <class Datagram, class Addr>
class BaseDatagram : public io::detail::Fd,
                     public ImplAsyncSend<BaseDatagram<Datagram, Addr>, Addr>,
                     public ImplAsyncRecv<BaseDatagram<Datagram, Addr>, Addr>,
                     public ImplLocalAddr<BaseDatagram<Datagram, Addr>, Addr>,
                     public ImplPeerAddr<BaseDatagram<Datagram, Addr>, Addr> {
protected:
    explicit BaseDatagram(const int fd)
        : Fd{fd} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto connect(const Addr &addr) noexcept {
        return io::detail::Connect{fd_, addr.sockaddr(), addr.length()};
    }

public:
    [[nodiscard]]
    static auto bind(const Addr &addr) -> Result<Datagram> {
        auto fd = ::socket(addr.family(), SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (fd < 0) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        if (::bind(fd, addr.sockaddr(), addr.length()) == -1) [[unlikely]] {
            ::close(fd);
            return std::unexpected{make_sys_error(errno)};
        }
        return Datagram{fd};
    }
};

} // namespace zedio::socket::detail
