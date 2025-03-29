#pragma once

namespace zedio::socket::detail {

template <class T, class Addr>
struct ImplPeerAddr {
    [[nodiscard]]
    auto peer_addr(this const T &self) noexcept -> Result<Addr> {
        Addr        addr{};
        std::size_t len{sizeof(addr)};
        if (::getpeername(self.handle(), addr.sockaddr(), &len) != 0) [[unlikely]] {
            return std::unexpected{make_socket_error()};
        }
        return addr;
    }
};

} // namespace zedio::socket::detail