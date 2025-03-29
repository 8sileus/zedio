#pragma once

#include "zedio/io/base/io_request.hpp"

// C++
#include <limits>

namespace zedio::io {

namespace detail {

    class WriteFile : public IORequest<decltype(::WriteFile)> {
    public:
        WriteFile(HANDLE f_handle, const void *buf, std::size_t nbytes, std::size_t offset)
            : IORequest{::WriteFile} {

            if (offset == 0) {
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

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto
write_file(HANDLE f_handle, const void *buf, std::size_t nbytes, std::size_t offset = -1) {
    return detail::WriteFile(f_handle, buf, nbytes, offset);
}

} // namespace zedio::io
