#pragma once

#include "zedio/common/concepts.hpp"
#include "zedio/io/io.hpp"

#ifdef __linux__
#define INVALID_SOCKET (~0)
#endif

namespace zedio::socket::detail {

class Socket {
public:
    explicit Socket(SocketHandle socket_handle)
        : socket_handle{socket_handle} {}

    ~Socket() {
        if (socket_handle != INVALID_SOCKET) {
            closesocket(socket_handle);
        }
    }

    Socket(Socket &&other)
        : socket_handle{other.socket_handle} {
        other.socket_handle = INVALID_SOCKET;
    }

    Socket &operator=(Socket &&other) noexcept {
        if (this->socket_handle != INVALID_SOCKET) {
            closesocket(this->socket_handle);
        }
        this->socket_handle = other.socket_handle;
        other.socket_handle = INVALID_SOCKET;
        return *this;
    }

    // uncopyable
    Socket(const Socket &other) = delete;
    Socket &operator=(const Socket &other) = delete;

public:
    [[nodiscard]]
    auto handle(this const Socket &self) noexcept {
        return self.socket_handle;
    }

    [[nodiscard]]
    auto take_handle(this Socket &self) noexcept {
        auto result = self.socket_handle;
        self.socket_handle = INVALID_SOCKET;
        return result;
    }

    template <typename Addr>
        requires is_socket_address<Addr>
    [[nodiscard]]
    auto bind(this Socket &self, const Addr &addr) -> Result<void> {
        if (::bind(self.socket_handle, addr.sockaddr(), addr.length()) != 0) [[unlikely]] {
            return std::unexpected{make_socket_error()};
        }
        return {};
    }

    [[nodiscard]]
    auto listen(this Socket &self, int maxn = SOMAXCONN) -> Result<void> {
        if (::listen(self.socket_handle, maxn) != 0) [[unlikely]] {
            return std::unexpected{make_socket_error()};
        }
        return {};
    }

    // [[REMEMBER_CO_AWAIT]]
    // auto shutdown(io::ShutdownBehavior how) noexcept {
    //     return io::detail::Shutdown{fd(), how};
    // }

public:
    template <typename T = Socket>
    [[nodiscard]]
    static auto create(int domain, int type, int protocol) -> Result<T> {
#ifdef __linux__
        type |= SOCK_NONBLOCK;
#endif
        auto handle = ::socket(domain, type, protocol);
#ifdef _WIN32
        u_long on = 1;
        ::ioctlsocket(handle, FIONBIO, &on);
#endif
        if (handle == INVALID_SOCKET) [[unlikely]] {
            return std::unexpected{make_socket_error()};
        }
        return T{Socket{handle}};
    }

private:
    SocketHandle socket_handle;
};

#ifndef __linux__
#undef INVALID_SOCKET
#endif

} // namespace zedio::socket::detail
