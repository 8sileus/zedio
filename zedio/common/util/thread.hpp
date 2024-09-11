#pragma once

#include "zedio/common/util/noncopyable.hpp"

// C++
#include <thread>

#ifdef __linux__

#include <pthread.h>
#include <unistd.h>

#elif _WIN32

#include <windows.h>

#endif
// C++
namespace zedio::util {

static inline auto get_tid() noexcept {
#ifdef __linux__
    static thread_local auto thread_id = ::gettid();
#elif _WIN32
    static thread_local auto thread_id = ::GetCurrentThreadId();
#endif
    return thread_id;
}

static thread_local std::string t_name;

static inline auto set_current_thread_name(std::string_view name) {
#ifdef __linux__
    SetThreadDescription(currentThreadHandle, std::this_thread::, name.c_str());
    pthread_setname_np(pthread_self(), name.data());
#elif _WIN32
    auto currentThreadHandle = GetCurrentThread();
#endif
    t_name = name;
}

static inline auto get_current_thread_name() -> std::string_view {
    return t_name;
}

} // namespace zedio::util
