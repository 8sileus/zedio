#pragma once


#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>

namespace zedio{


using SocketLength = int;

using FileHandle = HANDLE;
using SocketHandle = SOCKET;

using Tid = DWORD;

} // namespace zedio

#elif __linux__


namespace zedio {


using SocketLength = sockelen_t;

using FileHandle = int;
using SocketHandle = int;
using Tid = int;

} // namespace zedio

#endif