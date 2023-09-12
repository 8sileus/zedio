
#include <functional>

#include "log/log.hpp"

namespace zed {

class Executor;

class Fd {
public:
    using CallBack = std::function<void()>;

    explicit Fd(int fd, Executor* executor);

    ~Fd();

    [[nodiscard]] auto number() const noexcept -> int { return m_fd; }
    [[nodiscard]] auto executor() const noexcept -> Executor* { return m_executor; }
    [[nodiscard]] auto events() const noexcept -> int { return m_events; }

    // for executor to call
    void enroll_events(int events);
    void unenroll_events(int events);
    void clear_events() { m_events = 0; }

    void remove_from_executor();

    void set_executor(Executor* executor) { m_executor = executor; }
    void set_read_callback(CallBack callback) { m_read_callback = std::move(callback); }
    void set_write_callback(CallBack callback) { m_write_callback = std::move(callback); }

private:
    void handle_event(int revents);
    void update();

private:
    int       m_fd{-1};
    int       m_events{0};
    Executor* m_executor{nullptr};
    CallBack  m_read_callback{nullptr};
    CallBack  m_write_callback{nullptr};
};

}  // namespace zed
