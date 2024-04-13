#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/io/io.hpp"

namespace zedio::socket::detail {

class Socket : public io::detail::FD {
public:
    explicit Socket(const int fd)
        : FD{fd} {}



public:
    template <typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto bind(const Addr &addr) -> Result<void> {
        if (::bind(fd_, addr.sockaddr(), addr.length()) != 0) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    [[nodiscard]]
    auto listen(int maxn = SOMAXCONN) -> Result<void> {
        if (::listen(fd_, maxn) != 0) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return {};
    }

    [[REMEMBER_CO_AWAIT]]
    auto shutdown(io::ShutdownBehavior how) noexcept {
        return io::detail::Shutdown{fd(), how};
    }

public:
    template <typename T = Socket>
    [[nodiscard]]
    static auto create(const int domain, const int type, const int protocol) -> Result<T> {
        auto fd = ::socket(domain, type, protocol);
        if (fd < 0) [[unlikely]] {
            return std::unexpected{make_sys_error(errno)};
        }
        return T{Socket{fd}};
    }
};

} // namespace zedio::socket::detail
