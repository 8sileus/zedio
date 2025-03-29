#pragma once

// net
#include "zedio/socket/net/datagram.hpp"
#include "zedio/socket/net/listener.hpp"
#include "zedio/socket/net/socket.hpp"
#include "zedio/socket/net/stream.hpp"

namespace zedio::net {
using namespace zedio::socket::net;
}

#ifdef __linux__
// unix domain
#include "zedio/socket/unix_domain/datagram.hpp"
#include "zedio/socket/unix_domain/listener.hpp"
#include "zedio/socket/unix_domain/stream.hpp"

namespace zedio::unix_domian {
using namespace zedio::socket::unix_domain;
}
#endif
