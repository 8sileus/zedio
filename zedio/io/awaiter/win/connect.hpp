#pragma once

#include "zedio/io/base/io_request.hpp"

#include <MSWSock.h>

namespace zedio::io {

namespace detail {

    class Connect : public IORequest<LPFN_CONNECTEX> {
    public:
        Connect(SOCKET s, const sockaddr *addr, const SocketLength addrlen)
            : IORequest{get_async_connect(s)} {
            args = {
                s,
                addr,
                addrlen,
                nullptr,
                0,
                nullptr,
                &data.overlapped,
            };
        }

        auto await_resume() noexcept -> Result<void> {
            auto result = IORequest::await_resume();
            if (!result) [[unlikely]] {
                return std::unexpected{result.error()};
            }
            return {};
        }

    private:
        auto get_async_connect(SOCKET s) -> LPFN_CONNECTEX {
            LPFN_CONNECTEX ConnectEx;
            DWORD          dwBytes;
            GUID           guidConnectEx = WSAID_CONNECTEX;
            if (SOCKET_ERROR
                == WSAIoctl(s,
                            SIO_GET_EXTENSION_FUNCTION_POINTER,
                            &guidConnectEx,
                            sizeof(guidConnectEx),
                            &ConnectEx,
                            sizeof(ConnectEx),
                            &dwBytes,
                            NULL,
                            NULL)) {
                return ConnectEx;
            }
            return ConnectEx;
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto connect(SOCKET s, const sockaddr *addr, const SocketLength addrlen) {
    return detail::Connect{s, addr, addrlen};
}

} // namespace zedio::io
