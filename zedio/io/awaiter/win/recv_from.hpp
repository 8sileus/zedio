#pragma once

#include "zedio/io/base/io_request.hpp"

namespace zedio::io {

namespace detail {

    class RecvFrom : public IORequest<decltype(WSARecvFrom)> {
    public:
        RecvFrom(SOCKET            s,
                 char             *buf,
                 const std::size_t len,
                 const DWORD       flags,
                 sockaddr         *addr,
                 SocketLength     *addrlen)
            : IORequest{WSARecvFrom}
            , flags{flags} {
            
            wsa_buf.buf = buf;
            wsa_buf.len = static_cast<ULONG>(len);
            args = {s,
                    &wsa_buf,
                    1,
                    &data.result,
                    &this->flags,
                    addr,
                    addrlen,
                    &data.overlapped,
                    nullptr};
        }

    private:
        WSABUF wsa_buf{};
        DWORD  flags;
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto recvfrom(SOCKET            s,
                            char             *buf,
                            const std::size_t len,
                            const DWORD       flags,
                            sockaddr         *addr,
                            SocketLength     *addrlen) {
    return detail::RecvFrom{s, buf, len, flags, addr, addrlen};
}

} // namespace zedio::io
