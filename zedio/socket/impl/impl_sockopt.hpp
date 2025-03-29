#pragma once

#include "zedio/io/io.hpp"

#ifdef __linux__
// Linux
#include <netinet/in.h>
#include <netinet/tcp.h>
#elif _WIN32
#include <ws2tcpip.h>
#endif

namespace zedio::socket::detail {

static inline auto set_sock_opt(SocketHandle handle,
                                int          level,
                                int          optname,
#ifdef __linux__
                                const void *optval,
#elif _WIN32
                                const char *optval,
#endif
                                socklen_t optlen) -> Result<void> {
    if (::setsockopt(handle, level, optname, optval, optlen) != 0) [[unlikely]] {
        return std::unexpected{make_socket_error()};
    }
    return {};
}

static inline auto get_sock_opt(SocketHandle handle,
                                int          level,
                                int          optname,
#ifdef __linux__
                                void *optval,
#elif _WIN32
                                char *optval,
#endif
                                socklen_t optlen) -> Result<void> {
    if (::getsockopt(handle, level, optname, optval, &optlen) != 0) [[unlikely]] {
        return std::unexpected{make_socket_error()};
    }
    return {};
}

template <class T>
struct ImplNodelay {
    [[nodiscard]]
    auto set_nodelay(this T &self, bool on) noexcept {
#ifdef __linux__
        int optval{on ? 1 : 0};
        return set_sock_opt(self.handle(), SOL_TCP, TCP_NODELAY, &optval, sizeof(optval));
#elif _WIN32
        DWORD optval{on ? 1 : 0};
        return set_sock_opt(self.handle(), IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
#endif
    }

    [[nodiscard]]
    auto nodelay(this const T &self) noexcept -> Result<bool> {
#ifdef __linux__
        int  optval{0};
        auto ret = get_sock_opt(self.handle(), SOL_TCP, TCP_NODELAY, &optval, sizeof(optval));
#elif _WIN32
        DWORD optval{0};
        auto  ret = get_sock_opt(self.handle(), IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
#endif
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return optval != 0;
    }
};

template <class T>
struct ImplPasscred {
#ifdef __linux__
    [[nodiscard]]
    auto set_passcred(this T &self, bool on) noexcept {
        int optval{on ? 1 : 0};
        return set_sock_opt(self.handle(), SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto passcred() const noexcept -> Result<bool> {
        int optval{0};
        if (auto ret = get_sock_opt(static_cast<const T *>(this)->fd(),
                                    SOL_SOCKET,
                                    SO_PASSCRED,
                                    &optval,
                                    sizeof(optval));
            ret) [[likely]]
        {
            return optval != 0;
        } else {
            return std::unexpected{ret.error()};
        }
    }
#endif
};

template <class T>
struct ImplRecvBufSize {
    [[nodiscard]]
    auto set_recv_buffer_size(this T &self, int size) noexcept {
        return set_sock_opt(self.handle(), SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    }

    [[nodiscard]]
    auto recv_buffer_size(this const T &self) noexcept -> Result<std::size_t> {
        int  size{0};
        auto ret = get_sock_opt(self.handle(), SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return static_cast<std::size_t>(size);
    }
};

template <class T>
struct ImplSendBufSize {
    [[nodiscard]]
    auto set_send_buffer_size(this T &self, int size) noexcept {
        return set_sock_opt(self.handle(), SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    }

    [[nodiscard]]
    auto send_buffer_size(this const T &self) noexcept -> Result<std::size_t> {
        int  size{0};
        auto ret = get_sock_opt(self.handle(), SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return static_cast<std::size_t>(size);
    }
};

template <class T>
struct ImplKeepalive {
    [[nodiscard]]
    auto set_keepalive(this T &self, bool on) noexcept {
        int optval{on ? 1 : 0};
        return set_sock_opt(self.handle(), SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto keepalive(this const T &self) noexcept -> Result<bool> {
        int  optval{0};
        auto ret = get_sock_opt(self.handle(), SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return optval != 0;
    }
};

template <class T>
struct ImplLinger {
    [[nodiscard]]
    auto set_linger(this T &self, std::optional<std::chrono::seconds> duration) noexcept {
        struct linger lin {
            .l_onoff{0}, .l_linger{0},
        };
        if (duration.has_value()) {
            lin.l_onoff = 1;
            lin.l_linger = duration.value().count();
        }
        return set_sock_opt(self.handle(), SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
    }

    [[nodiscard]]
    auto linger(this const T &self) noexcept -> Result<std::optional<std::chrono::seconds>> {
        struct linger lin;
        auto          ret = get_sock_opt(self.handle(), SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        if (lin.l_onoff == 0) {
            return std::nullopt;
        } else {
            return std::chrono::seconds(lin.l_linger);
        }
    }
};

template <class T>
struct ImplBoradcast {
    [[nodiscard]]
    auto set_broadcast(this T &self, bool on) noexcept {
        int optval{on ? 1 : 0};
        return set_sock_opt(self.handle(), SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto broadcast(this const T &self) noexcept -> Result<bool> {
        int  optval{0};
        auto ret = get_sock_opt(self.handle(), SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return optval != 0;
    }
};

template <class T>
struct ImplTTL {
    [[nodiscard]]
    auto set_ttl(this T &self, uint32_t ttl) noexcept {
        return set_sock_opt(self.handle(), IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    }

    [[nodiscard]]
    auto ttl(this const T &self) noexcept -> Result<uint32_t> {
        uint32_t optval{0};
        auto     ret = get_sock_opt(self.handle(), IPPROTO_IP, IP_TTL, &optval, sizeof(optval));
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return optval;
    }
};

template <class T>
struct ImplReuseAddr {
    [[nodiscard]]
    auto set_reuseaddr(this T &self, bool on) noexcept {
        int optval{on ? 1 : 0};
        return set_sock_opt(self.handle(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto reuseaddr(this const T &self) noexcept -> Result<bool> {
        int  optval{0};
        auto ret = get_sock_opt(self.handle(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return optval != 0;
    }
};

template <class T>
struct ImplReusePort {

#ifdef __linux__
    [[nodiscard]]
    auto set_reuseport(this T &self, bool on) noexcept -> Result<void> {
        auto optval{on ? 1 : 0};
        return set_sock_opt(self.handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    }

    [[nodiscard]]
    auto reuseport(this const T &self) noexcept -> Result<bool> {
        auto optval{0};
        auto ret = get_sock_opt(self.handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        if (!ret) [[unlikely]] {
            return std::unexpected{ret.error()};
        }
        return optval != 0;
    }
#endif
};

} // namespace zedio::socket::detail
