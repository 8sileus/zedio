#pragma once

#include <cassert>
#include <exception>
#include <format>
#include <sstream>

#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>

namespace zed::util {

static auto EventsToString(int events) -> std::string {
    std::ostringstream ss;
#define XX(EVENTS)            \
    if (events & EVENTS) {    \
        ss << #EVENTS << " "; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP)
    XX(EPOLLERR);
#undef XX
    return ss.str();
}

static void SetNonBlocking(int fd) {
    assert(fd >= 0);
    int flag = ::fcntl(fd, F_GETFL, 0);
    if (flag & O_NONBLOCK) [[unlikely]] {
        return;
    }
    int state = ::fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    if (state == -1) [[unlikely]] {
        throw std::runtime_error(std::format("fd:{} set non_block failed {}", fd, strerror(errno)));
    }
}
}  // namespace zed::util
