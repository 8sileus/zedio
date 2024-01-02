#pragma once

#include "async/awaiter_io.hpp"
#include "async/awaiter_time.hpp"

namespace zed::async {

// timeout

using sleep = detail::SleepAwaiter;

template <detail::is_awaiter IOAwaiter>
using timeout = detail::TimeoutAwaiter<IOAwaiter>;

// async i/o

using detail::AccessLevel;

template <AccessLevel level = AccessLevel::Distributive>
using socket = detail::SocketAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using accept = detail::AcceptAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using connect = detail::ConnectAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using shutdown = detail::ShutdownAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using close = detail::CloseAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using recv = detail::RecvAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using recvmsg = detail::RecvMsgAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using send = detail::SendAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using sendmsg = detail::SendMsgAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using sendto = detail::SendToAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using read = detail::ReadAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using readv = detail::ReadvAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using readv2 = detail::Readv2Awaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using write = detail::WriteAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using writev = detail::WritevAwaiter<level>;

template <AccessLevel level = AccessLevel::Distributive>
using writev2 = detail::Writev2Awaiter<level>;

} // namespace zed::async
