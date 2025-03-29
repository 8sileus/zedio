#pragma once

#include "zedio/io/base/io_request.hpp"

namespace zedio::io {

namespace detail {

    class SendTo : public IORequest<decltype(WSASendTo)> {
    public:
        SendTo(SOCKET             s,
               const char        *buf,
               const std::size_t  len,
               const DWORD        flags,
               const sockaddr    *addr,
               const SocketLength addrlen)
            : IORequest{WSASendTo} {

            wsa_buf.buf = const_cast<char *>(buf);
            wsa_buf.len = static_cast<ULONG>(len);
            args = {s, &wsa_buf, 1, &data.result, flags, addr, addrlen, &data.overlapped, nullptr};
        }

    private:
        WSABUF wsa_buf{};
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto sendto(SOCKET      s,
                          const char *buf,
                          const std::size_t  len,
                          const DWORD        flags,
                          const sockaddr    *addr,
                          const SocketLength addrlen) {
    return detail::SendTo{s, buf, len, flags, addr, addrlen};
}

} // namespace zedio::io
