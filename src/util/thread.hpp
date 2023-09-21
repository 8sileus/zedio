#pragma once

#include "util/noncopyable.hpp"
// Linux
#include <pthread.h>
#include <unistd.h>

namespace zed::this_thread {

auto get_tid() noexcept -> pid_t {
    static thread_local pid_t t_tid = ::gettid();
    return t_tid;
}
} // namespace zed::this_thread

namespace zed::util {
class SpinMutex : Noncopyable {
public:
    explicit SpinMutex(int pshared = 0) noexcept { pthread_spin_init(&m_mutex, pshared); }
    ~SpinMutex() noexcept { pthread_spin_destroy(&m_mutex); }

    void lock() noexcept { pthread_spin_lock(&m_mutex); }
    void unlock() noexcept { pthread_spin_unlock(&m_mutex); }

private:
    pthread_spinlock_t m_mutex;
};

} // namespace zed::util
