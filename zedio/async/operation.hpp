#pragma once

#include "zedio/async/io/awaiter.hpp"
#include "zedio/async/time/awaiter.hpp"

namespace zedio::async {

// timeout

using sleep = detail::SleepAwaiter;

template <is_awaiter IO>
using timeout = detail::TimeoutAwaiter<IO>;

// async i/o

using detail::Mode;

template <Mode mode = Mode::S>
using socket = detail::SocketAwaiter<mode>;

template <Mode mode = Mode::S>
using accept = detail::AcceptAwaiter<mode>;

template <Mode mode = Mode::S>
using connect = detail::ConnectAwaiter<mode>;

template <Mode mode = Mode::S>
using shutdown = detail::ShutdownAwaiter<mode>;

template <Mode mode = Mode::S>
using close = detail::CloseAwaiter<mode>;

template <Mode mode = Mode::S>
using recv = detail::RecvAwaiter<mode>;

template <Mode mode = Mode::S>
using recvmsg = detail::RecvMsgAwaiter<mode>;

template <Mode mode = Mode::S>
using send = detail::SendAwaiter<mode>;

template <Mode mode = Mode::S>
using sendmsg = detail::SendMsgAwaiter<mode>;

template <Mode mode = Mode::S>
using sendto = detail::SendToAwaiter<mode>;

template <Mode mode = Mode::S>
using read = detail::ReadAwaiter<mode>;

template <Mode mode = Mode::S>
using readv = detail::ReadvAwaiter<mode>;

// template <Mode mode=Mode::S>
// using readv2 = detail::Readv2Awaiter<mode>;

template <Mode mode = Mode::S>
using write = detail::WriteAwaiter<mode>;

template <Mode mode = Mode::S>
using writev = detail::WritevAwaiter<mode>;

// template <Mode mode=Mode::S>
// using writev2 = detail::Writev2Awaiter<mode>;

template <Mode mode = Mode::S>
using openat2 = detail::OpenAt2Awaiter<mode>;

template <Mode mode = Mode::S>
using openat = detail::OpenAtAwaiter<mode>;

// template <Mode mode = Mode::S>
// using statx = detail::StatxAwaiter<mode>;

template <Mode mode = Mode::S>
using fsync = detail::FsyncAwaiter<mode>;
} // namespace zedio::async
