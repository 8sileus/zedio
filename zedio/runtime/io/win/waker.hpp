#pragma once

#include "zedio/runtime/io/win/poller.hpp"

// WIN
#include <WinSock2.h>
#include <windows.h>

namespace zedio::runtime::detail {

class Waker {
public:
    Waker(Poller &poller)
        : poller{poller} {}

public:
    void wake_up() {
        PostQueuedCompletionStatus(poller.get_iocp(), 0, 0, nullptr);
    }

    void turn_on() {}

private:
    Poller &poller;
};

} // namespace zedio::runtime::detail