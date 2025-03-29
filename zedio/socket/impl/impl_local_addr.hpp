#pragma once

namespace zedio::socket::detail {

template <class T, class Addr>
struct ImplLocalAddr {
    [[nodiscard]]
    auto local_addr(this const T &self) noexcept -> Result<Addr> {
        Addr        addr{};
        std::size_t len{sizeof(addr)};
        if (::getsockname(self.handle(), addr.sockaddr(), &len) != 0) [[unlikely]] {
            return std::unexpected{make_socket_error()};
        }
        return addr;
    }
};

} // namespace zedio::socket::detail