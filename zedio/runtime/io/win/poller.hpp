#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/runtime/config.hpp"
#include "zedio/runtime/io/win/completion.hpp"
// C
#include <cstring>
// C++
#include <chrono>
#include <format>
// Win
#include <WinSock2.h>
#include <windows.h>

namespace zedio::runtime::detail {
class Poller;

inline thread_local Poller *t_ring{nullptr};

class Poller {
public:
    Poller(const Config &config) {
        iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
        if (iocp == nullptr) [[unlikely]] {
            throw std::runtime_error(
                std::format("Initialize IOCP faield, error: {}.", strerror(errno)));
        }
        assert(t_ring == nullptr);
        t_ring = this;
    }

    ~Poller() {
        CloseHandle(iocp);
        t_ring = nullptr;
    }

public:
    [[nodiscard]]
    auto get_iocp(this Poller &poller) -> HANDLE {
        return poller.iocp;
    }

    [[nodiscard]]
    auto peek_batch(this Poller &poller, std::span<IOCompletion> completions) -> std::size_t {
        ULONG result = 0;
        if (!GetQueuedCompletionStatusEx(poller.iocp,
                                         reinterpret_cast<LPOVERLAPPED_ENTRY>(completions.data()),
                                         completions.size(),
                                         &result,
                                         0,
                                         false)) [[unlikely]] {
            // TODO log error
        }
        return result;
    }

    void wait(std::optional<time_t> timeout) {
        time_t ms = timeout.has_value() ? timeout.value() / 1000 : INFINITE;
        if (!GetQueuedCompletionStatusEx(iocp, nullptr, 1, nullptr, ms, false)) {
            // TODO log error
        }
    }

    void consume(std::size_t cnt) {}

    void submit() {}

    void force_submit() {}

private:
    HANDLE iocp;
};

} // namespace zedio::runtime::detail
