#pragma once

// Win
#include <WinSock2.h>
#include <windows.h>

namespace zedio::io::detail {
struct IOData;
} // namespace zedio::io::detail

namespace zedio::runtime::detail {

struct IOCompletion {
    int get_result(this IOCompletion &self) {
        return self.entry.dwNumberOfBytesTransferred;
    }

    io::detail::IOData *get_data(this IOCompletion &self) {
        return reinterpret_cast<io::detail::IOData *>(self.entry.lpOverlapped);
    }

    OVERLAPPED_ENTRY entry;
};

} // namespace zedio::runtime::detail
