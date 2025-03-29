#pragma once

#include "zedio/io/base/io_request.hpp"
// Win
#include <mswsock.h>

namespace zedio::io {

namespace detail {

    class Accept : public IORequest<decltype(AcceptEx)> {
    public:
        Accept(SOCKET listen_socket, sockaddr *addr, SocketLength *addrlen)
            : IORequest{AcceptEx}
            , addr{addr}
            , addrlen{addrlen} {
                
            args = {
                listen_socket,
                accept_socket,
                &buf,
                0,
                0,
                *addrlen,
                &data.result,
                &data.overlapped,
            };
        }

        auto await_resume(this Accept &self) noexcept -> Result<SocketHandle> {
            auto result = self.IORequest::await_resume();
            if (!result) [[unlikely]] {
                return std::unexpected{result.error()};
            }
            return self.accept_socket;
        }

    private:
        char         buf[128];
        SocketHandle accept_socket{};
        int         *addrlen;

    protected:
        sockaddr *addr;
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto accept(SOCKET listen_socket, sockaddr *addr, SocketLength *addrlen) {
    return detail::Accept{listen_socket, addr, addrlen};
}

} // namespace zedio::io
