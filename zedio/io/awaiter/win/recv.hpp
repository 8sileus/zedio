#pragma once

#include "zedio/io/base/io_request.hpp"

namespace zedio::io {

namespace detail {

    class Recv : public IORequest<decltype(WSARecv)> {
    public:
        Recv(SOCKET s, char *buf, const std::size_t len, const DWORD flags)
            : IORequest{WSARecv}
            , flags{flags} {


            wsa_buf.buf = buf;
            wsa_buf.len = static_cast<ULONG>(len);

            args = {s, &wsa_buf, 1, &data.result, &this->flags, &data.overlapped, nullptr};
        }

    private:
        WSABUF wsa_buf{};
        DWORD  flags;
    };

    template <typename Buf>
        requires constructible_to_char_slice<Buf>
    class RecvVectored : public IORequest<decltype(WSARecv)> {
    public:
        RecvVectored(SOCKET s, std::span<Buf> bufs, DWORD flags)
            : IORequest{WSARecv}
            , flags{flags} {
            wsa_bufs.resize(bufs.size());

            for (auto i = 0; i < bufs.size(); i += 1) {
                auto buf = std::span{bufs[i]};
                wsa_bufs[i].buf = buf.data();
                wsa_bufs[i].len = static_cast<ULONG>(buf.size_bytes());
            }

            args = IORequest::ArgsTuple{s,
                                        wsa_bufs.data(),
                                        wsa_bufs.size(),
                                        &data.result,
                                        &this->flags,
                                        &data.overlapped,
                                        nullptr};
        }

    private:
        std::vector<WSABUF> wsa_bufs{};
        DWORD               flags;
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto recv(SOCKET s, char *buf, const std::size_t len, const DWORD flags) {
    return detail::Recv{s, buf, len, flags};
}

template <typename Buf>
    requires constructible_to_char_slice<Buf>
[[REMEMBER_CO_AWAIT]]
static inline auto recv_vectored(SOCKET s, std::span<Buf> bufs, const DWORD flags) {
    return detail::RecvVectored{s, bufs, flags};
}

} // namespace zedio::io
