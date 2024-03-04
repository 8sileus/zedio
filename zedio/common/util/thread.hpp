#pragma once

#include "zedio/common/util/noncopyable.hpp"
// Linux
#include <pthread.h>
#include <unistd.h>
// C++
#include <thread>

namespace zedio::current_thread {

auto get_tid() noexcept -> pid_t {
    static thread_local pid_t t_id = ::gettid();
    return t_id;
}

static thread_local std::string t_name;

auto set_thread_name(std::string_view name) {
    t_name = name;
}

auto get_thread_name() -> std::string_view {
    return t_name;
}

} // namespace zedio::current_thread

namespace zedio::util {
class SpinMutex : Noncopyable {
public:
    explicit SpinMutex(int pshared = 0) noexcept {
        pthread_spin_init(&mutex_, pshared);
    }
    ~SpinMutex() noexcept {
        pthread_spin_destroy(&mutex_);
    }

    void lock() noexcept {
        pthread_spin_lock(&mutex_);
    }
    void unlock() noexcept {
        pthread_spin_unlock(&mutex_);
    }

private:
    pthread_spinlock_t mutex_;
};

} // namespace zedio::util
