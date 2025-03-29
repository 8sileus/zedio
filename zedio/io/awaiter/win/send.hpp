#pragma once

#include "zedio/io/base/io_request.hpp"

namespace zedio::io {

namespace detail {

    class Send : public IORequest<decltype(WSASend)> {
    private:
        using Super = IORequest<decltype(WSASend)>;

    public:
        Send(SOCKET s, const char *buf, const std::size_t len, const DWORD flags)
            : IORequest{WSASend} {

            wsa_buf.buf = const_cast<char *>(buf);
            wsa_buf.len = static_cast<int>(len);

            args = {s, &wsa_buf, 1, &data.result, flags, &data.overlapped, nullptr};
        }

    private:
        WSABUF wsa_buf{};
    };

    template <typename Buf>
        requires constructible_to_char_slice<Buf>
    class SendVectored : public IORequest<decltype(WSASend)> {
    public:
        SendVectored(SOCKET s, std::span<Buf> bufs, const DWORD flags)
            : IORequest{WSASend} {
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
                                        flags,
                                        &data.overlapped,
                                        nullptr};
        }

    private:
        std::vector<WSABUF> wsa_bufs{};
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto send(SOCKET s, const char *buf, const std::size_t len, const DWORD flags) {
    return detail::Send{s, buf, len, flags};
}

template <typename Buf>
    requires constructible_to_char_slice<Buf>
[[REMEMBER_CO_AWAIT]]
static inline auto send_vectored(SOCKET s, std::span<Buf> bufs, const DWORD flags) {
    return detail::SendVectored{s, bufs, flags};
}

} // namespace zedio::io
