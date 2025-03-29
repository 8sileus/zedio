#pragma once

#include "zedio/io/base/io_request.hpp"

namespace zedio::io {

namespace detail {

    class ReadFile : public IORequest<decltype(::ReadFile)> {
    public:
        ReadFile(HANDLE f_handle, void *buf, std::size_t nbytes, std::size_t offset)
            : IORequest{::ReadFile} {

            if (offset == -1) {
                DWORD low;
                LONG  high;
                low = SetFilePointer(f_handle, 0, &high, FILE_CURRENT);
                data.overlapped.Offset = low;
                data.overlapped.OffsetHigh = high;
            } else {
                constexpr DWORD MASK = 0xFFFFFFFF;
                data.overlapped.Offset = offset & MASK;
                data.overlapped.OffsetHigh = (offset >> 32) & MASK;
            }
            
            auto len = static_cast<DWORD>(nbytes);
            if (nbytes >= std::numeric_limits<DWORD>::max()) [[unlikely]] {
                len = std::numeric_limits<DWORD>::max();
            }

            args = {f_handle, buf, len, &data.result, &data.overlapped};
        }
    };

    template <typename Buf>
        requires constructible_to_char_slice<Buf>
    class ReadFileVectored : public ReadFile {
    public:
        ReadFileVectored(HANDLE f_handle, std::span<Buf> bufs, std::size_t offset)
            : ReadFile{} {

            if (offset == -1) {
                DWORD low;
                LONG  high;
                low = SetFilePointer(f_handle, 0, &high, FILE_CURRENT);
                data.overlapped.Offset = low;
                data.overlapped.OffsetHigh = high;
            } else {
                constexpr DWORD MASK = 0xFFFFFFFF;
                data.overlapped.Offset = offset & MASK;
                data.overlapped.OffsetHigh = (offset >> 32) & MASK;
            }

            auto len = static_cast<DWORD>(nbytes);
            if (nbytes >= std::numeric_limits<DWORD>::max()) [[unlikely]] {
                len = std::numeric_limits<DWORD>::max();
            }

            args = {f_handle, buf, len, &data.result, &data.overlapped};
        }

    private:
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto read_file(HANDLE f_handle, void *buf, std::size_t nbytes, std::size_t offset) {
    return detail::ReadFile{f_handle, buf, nbytes, offset};
}

} // namespace zedio::io
