#pragma once

#include "async/awaiter_io.hpp"
#include "async/awaiter_time.hpp"

namespace zed::async {

using sleep = detail::SleepAwaiter;

template <detail::is_awaiter IOAwaiter>
using timeout = detail::TimeoutAwaiter<IOAwaiter>;

using detail::AccessLevel;

template <AccessLevel level = AccessLevel::distributive>
using socket = detail::SocketAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using accept = detail::AcceptAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using connect = detail::ConnectAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using shutdown = detail::ShutdownAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using close = detail::CloseAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using recv = detail::RecvAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using recvmsg = detail::RecvMsgAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using send = detail::SendAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using sendmsg = detail::SendMsgAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using sendto = detail::SendToAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using read = detail::ReadAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using readv = detail::ReadvAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using readv2 = detail::Readv2Awaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using write = detail::WriteAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using writev = detail::WritevAwaiter<level>;

template <AccessLevel level = AccessLevel::distributive>
using writev2 = detail::Writev2Awaiter<level>;

} // namespace zed::async
