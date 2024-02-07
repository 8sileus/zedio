#pragma once

#include "zed/async/awaiter_io.hpp"
#include "zed/async/awaiter_time.hpp"

namespace zed::async {

// timeout

using sleep = detail::SleepAwaiter;

template <detail::is_awaiter IOAwaiter>
using timeout = detail::TimeoutAwaiter<IOAwaiter>;

// async i/o

using detail::OPFlag;

template <OPFlag flag = OPFlag::Distributive>
using socket = detail::SocketAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using accept = detail::AcceptAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using connect = detail::ConnectAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using shutdown = detail::ShutdownAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using close = detail::CloseAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using recv = detail::RecvAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using recvmsg = detail::RecvMsgAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using send = detail::SendAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using sendmsg = detail::SendMsgAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using sendto = detail::SendToAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using read = detail::ReadAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using readv = detail::ReadvAwaiter<flag>;

// template <OPFlag flag = OPFlag::Distributive>
// using readv2 = detail::Readv2Awaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using write = detail::WriteAwaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using writev = detail::WritevAwaiter<flag>;

// template <OPFlag flag = OPFlag::Distributive>
// using writev2 = detail::Writev2Awaiter<flag>;

template <OPFlag flag = OPFlag::Distributive>
using openat2 = detail::OpenAtAwaiter<flag>;

} // namespace zed::async
