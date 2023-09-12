#include "net/fd.h"

#include <sstream>

#include <fcntl.h>
#include <sys/epoll.h>

#include "net/executor.h"

namespace zed {

// for debug
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
        log::Debug("fd:{} has already set non block", fd);
        return;
    }
    int state = ::fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    if (state == -1) [[unlikely]] {
        throw std::runtime_error(
            std::format("fd:{} set non_block failed reason:{}", fd, strerror(errno)));
    }
}

Fd::Fd(int fd, Executor* executor) : m_fd{fd}, m_executor(executor) { SetNonBlocking(fd); }

Fd::~Fd() {
    // TODO executor remove this fd;
    ::close(m_fd);
}

void Fd::handle_event(int revents) {
    if (!(revents & EPOLLIN) && !(revents & EPOLLOUT)) [[unlikely]] {
        log::Error("fd:{} have nokown event:{}", m_fd, EventsToString(revents));
        return;
    }
    if (revents & EPOLLIN) {
        if (m_read_callback) [[likely]] {
            m_read_callback();
        }
    }
    if (revents & EPOLLOUT) {
        if (m_write_callback) [[likely]] {
            m_write_callback();
        }
    }
}

void Fd::enroll_events(int events) {
    if ((m_events & events) == events) {
        log::Debug("the events already exist");
        return;
    }
    m_events |= events;
    update();
}

void Fd::unenroll_events(int events) {
    if (!(m_events & events)) {
        log::Debug("the events don't exist");
        return;
    }
    m_events &= ~events;
    update();
}

void Fd::update() {
    epoll_event event;
    event.events = m_events;
    event.data.ptr = this;
    m_executor->update_event(m_fd, event);
}



}  // namespace zed
