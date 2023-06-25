#pragma once

#include "noncopyable.hpp"

#include <pthread.h>

namespace zed::util
{
class SpinMutex : Noncopyable {
public:
    explicit SpinMutex(int pshared = 0) noexcept { pthread_spin_init(&m_mutex, pshared); }

    ~SpinMutex() noexcept{ pthread_spin_destroy(&m_mutex); }

    void lock() noexcept { pthread_spin_lock(&m_mutex); }

    void unlock() noexcept { pthread_spin_unlock(&m_mutex); }

private:
    pthread_spinlock_t m_mutex;
};

} // namespace zed::util
