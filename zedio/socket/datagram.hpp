#pragma once

#include "zedio/io/impl/impl_datagram_read.hpp"
#include "zedio/io/impl/impl_datagram_write.hpp"
#include "zedio/socket/impl/impl_local_addr.hpp"
#include "zedio/socket/impl/impl_peer_addr.hpp"
#include "zedio/socket/socket.hpp"

namespace zedio::socket::detail {

template <class Datagram, class Addr>
class BaseDatagram : public io::detail::ImplDatagramRead<BaseDatagram<Datagram, Addr>, Addr>,
                     public io::detail::ImplDatagramWrite<BaseDatagram<Datagram, Addr>, Addr>,
                     public ImplLocalAddr<BaseDatagram<Datagram, Addr>, Addr>,
                     public ImplPeerAddr<BaseDatagram<Datagram, Addr>, Addr> {
protected:
    explicit BaseDatagram(Socket &&inner)
        : inner_{std::move(inner)} {}

public:
    [[REMEMBER_CO_AWAIT]]
    auto connect(this BaseDatagram &self, const Addr &addr) noexcept {
        return io::detail::Connect{self.handle(), addr.sockaddr(), addr.length()};
    }

    [[nodiscard]]
    auto close(this BaseDatagram &self) noexcept {
        return self.inner_.close();
    }

    [[nodiscard]]
    auto handle(this const BaseDatagram &self) noexcept {
        return self.inner.handle();
    }

public:
    [[nodiscard]]
    static auto bind(const Addr &addr) -> Result<Datagram> {
        auto ret = Socket::create<Datagram>(addr.family(), SOCK_DGRAM, 0);
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        if (::bind(ret.value().fd(), addr.sockaddr(), addr.length()) != 0) [[unlikely]] {
            return std::unexpected{make_system_error()};
        }
        return Datagram{std::move(ret.value())};
    }

private:
    Socket inner_;
};

} // namespace zedio::socket::detail
