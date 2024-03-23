#pragma once

namespace zedio::socket::detail {

template <class T, class Addr>
struct ImplPeerAddr {
    [[nodiscard]]
    auto peer_addr() const noexcept -> Result<Addr> {
        Addr      addr{};
        socklen_t len{sizeof(addr)};
        if (::getpeername(static_cast<const T *>(this)->fd(), addr.sockaddr(), &len) == -1)
            [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return addr;
    }
};

} // namespace zedio::socket::detail