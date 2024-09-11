#pragma once

// Win
#include <winsock2.h>

namespace zedio::io::detail {
struct IOData;
} // namespace zedio::io::detail

namespace zedio::runtime::detail {

struct IOCompletion {
    auto get_result(const this IOCompletion &self) -> int {
        return self.entry.dwNumberOfBytesTransferred;
    }

    auto get_data(const this IOCompletion &self) -> io::detail::IOData * {
        return reinterpret_cast<io::detail::IOData *>(self.entry.lpOverlapped);
    }

    OVERLAPPED_ENTRY entry;
};

} // namespace zedio::runtime::detail
