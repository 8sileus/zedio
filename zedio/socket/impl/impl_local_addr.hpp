#pragma once

namespace zedio::socket::detail {

template <class T, class Addr>
struct ImplLocalAddr {
    [[nodiscard]]
    auto local_addr() const noexcept -> Result<Addr> {
        Addr      addr{};
        socklen_t len{sizeof(addr)};
        if (::getsockname(static_cast<const T *>(this)->fd(), addr.sockaddr(), &len) == -1)
            [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return addr;
    }
};

} // namespace zedio::socket::detail