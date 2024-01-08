#pragma once

#include "zed/common/util/noncopyable.hpp"
// Linux
#include <pthread.h>
#include <unistd.h>
// C++
#include <thread>

namespace zed::current_thread {

auto get_tid() noexcept -> pid_t {
    static thread_local pid_t t_tid = ::gettid();
    return t_tid;
}
} // namespace zed::current_thread

namespace zed::util {
class SpinMutex : Noncopyable {
public:
    explicit SpinMutex(int pshared = 0) noexcept { pthread_spin_init(&mutex_, pshared); }
    ~SpinMutex() noexcept { pthread_spin_destroy(&mutex_); }

    void lock() noexcept { pthread_spin_lock(&mutex_); }
    void unlock() noexcept { pthread_spin_unlock(&mutex_); }

private:
    pthread_spinlock_t mutex_;
};

} // namespace zed::util
