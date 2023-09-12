#pragma once

#include <functional>

#include <fcntl.h>
#include <sys/epoll.h>

#include "log/log.hpp"
#include "net/executor.hpp"
#include "net/utility.hpp"

namespace zed {

class Fd {
public:
    using CallBack = std::function<void()>;

    explicit Fd::Fd(int fd, Executor* executor = GetCurrentExecutor())
        : m_fd{fd}, m_executor(executor) {
        util::SetNonBlocking(fd);
    }

    ~Fd() {
        unenroll();
        ::close(m_fd);
    }

    [[nodiscard]] auto number() const noexcept -> int { return m_fd; }
    [[nodiscard]] auto executor() const noexcept -> Executor* { return m_executor; }
    [[nodiscard]] auto events() const noexcept -> int { return m_events; }

    // for executor to call
    void add_events(int events) {
        if ((m_events & events) == events) {
            log::Debug("the events already exist");
            return;
        }
        m_events |= events;
        update_events();
    }

    void del_events(int events) {
        if (!(m_events & events)) {
            log::Debug("the events don't exist");
            return;
        }
        m_events &= ~events;
        update_events();
    }

    void unenroll() {
        if (m_in_epoll) {
            epoll_event event;
            m_executor->epoll_ctl<EPOLL_OP::DEL>(m_fd, event);
            m_in_epoll = false;
        }
    }

    void set_executor(Executor* executor) noexcept { m_executor = executor; }

    void set_read_callback(CallBack callback) { m_read_callback = std::move(callback); }

    void set_write_callback(CallBack callback) { m_write_callback = std::move(callback); }

    void handle_event(int revents) {
        if (!(revents & EPOLLIN) && !(revents & EPOLLOUT)) [[unlikely]] {
            log::Error("fd:{} have nokown event:{}", m_fd, util::EventsToString(revents));
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

private:
    void update_events() {
        assert(executor);

        epoll_event event;
        event.events = m_events;
        event.data.ptr = this;
        if (!m_in_epoll) [[unlikely]] {
            m_executor->epoll_ctl<EPOLL_OP::ADD>(m_fd, event);
            m_in_epoll = true;
        } else {
            m_executor->epoll_ctl<EPOLL_OP::MOD>(m_fd, event);
        }
    }

private:
    int       m_fd{-1};
    int       m_events{0};
    Executor* m_executor{nullptr};
    CallBack  m_read_callback{nullptr};
    CallBack  m_write_callback{nullptr};
    bool      m_in_epoll{false};
};

}  // namespace zed
