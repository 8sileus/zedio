#pragma once

// C++
#include <chrono>
#include <coroutine>

namespace zedio::runtime::detail {

class Entry;

} // namespace zedio::runtime::detail

namespace zedio::io::detail {

struct IOData {
    std::coroutine_handle<>               coroutine_handle{nullptr};
    runtime::detail::Entry               *entry{nullptr};
    std::chrono::steady_clock::time_point deadline;
#ifdef __linux__
    int result;
#elif _WIN32
    HANDLE     handle;
    DWORD      result;
    OVERLAPPED overlapped{};
#endif
};

} // namespace zedio::io::detail
