#pragma once

#include <WinSock2.h>

namespace zedio::io {

enum class ShutdownBehavior {
    Read = SD_RECEIVE,
    Write = SD_SEND,
    ReadWrite = SD_BOTH,
};

} // namespace zedio::io