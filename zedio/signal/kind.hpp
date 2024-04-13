#pragma once

#include "zedio/common/debug.hpp"

#ifdef __linux__

#include <signal.h>

#elif _WIN32 || _WIN64

// windows

#endif

// C++
#include <list>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace zedio::signal {

enum class SignalKind {
    Alarm = SIGALRM,
    Child = SIGCHLD,
    Hangup = SIGHUP,
    Interrupt = SIGINT,
    IO = SIGIO,
    Pipe = SIGPIPE,
    Quit = SIGQUIT,
    Terminate = SIGTERM,
};

[[nodiscard]]
static inline constexpr auto kind_to_string(SignalKind kind) -> std::string_view {
    switch (kind) {
        using enum SignalKind;
    case Alarm:
        return "Alarm";
    case Child:
        return "Child";
    case Hangup:
        return "Hangup";
    case Interrupt:
        return "Interrupt";
    case IO:
        return "IO";
    case Pipe:
        return "Pipe";
    case Quit:
        return "Quit";
    case Terminate:
        return "Terminate";
    default:
        std::unreachable();
    }
}

namespace detail {
    using HandleMap = std::unordered_map<SignalKind, std::list<std::coroutine_handle<>>>;
    thread_local static HandleMap s_handles{};

    template <SignalKind Kind>
    struct SignalHandler {
    public:
        SignalHandler() {
            if (::signal(static_cast<int>(Kind), notify_all) != 0) [[unlikely]] {
                LOG_ERROR("register signal handle for {} failed", kind_to_string(Kind));
            }
        }

        // ~SignalHandler() {
        //     if (sigaction(static_cast<int>(Kind), SIG_DFL, nullptr) != 0) [[unlikely]] {
        //         LOG_ERROR("unregister signal handle for {} failed", kind_to_string(Kind));
        //     }
        // }

    public:
        static void push_back(std::coroutine_handle<> handle) {
            auto lock = std::lock_guard{s_mutex};
            s_handles.push_back(handle);
        }

    private:
        static void notify_all(int n) {
            assert(n == static_cast<int>(Kind));

            auto handles = std::list<std::coroutine_handle<>>{};
            {
                auto lock = std::lock_guard{s_mutex};
                s_handles.swap(handles);
            }
            runtime::detail::schedule_remote(std::move(handles), handles.size());
        }

    private:
        static std::list<std::coroutine_handle<>> s_handles;
        static std::mutex                         s_mutex;
    };

    template <SignalKind Kind>
    std::list<std::coroutine_handle<>> SignalHandler<Kind>::s_handles{};

    template <SignalKind Kind>
    std::mutex SignalHandler<Kind>::s_mutex{};
} // namespace detail

} // namespace zedio::signal
