#pragma once

#include "zedio/common/util/noncopyable.hpp"
// Linux
#include <pthread.h>
#include <unistd.h>
// C++
#include <thread>

namespace zedio::util {

static inline auto get_tid() noexcept -> pid_t {
    static thread_local pid_t t_id = ::gettid();
    return t_id;
}

static thread_local std::string t_name;

static inline auto set_current_thread_name(std::string_view name) {
    pthread_setname_np(pthread_self(), name.data());
    t_name = name;
}

static inline auto get_current_thread_name() -> std::string_view {
    return t_name;
}

} // namespace zedio::util
