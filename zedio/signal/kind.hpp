#pragma once

#include "zedio/common/debug.hpp"
#include "zedio/runtime/runtime.hpp"

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
            struct sigaction act {};
            ::sigemptyset(&act.sa_mask);
            ::sigaddset(&act.sa_mask, static_cast<int>(Kind));
            act.sa_handler = &notify_all;
            act.sa_flags = 0;
            act.sa_restorer = nullptr;

            if (::sigaction(static_cast<int>(Kind), &act, nullptr) != 0) [[unlikely]] {
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
            if (runtime::detail::is_current_thread()) {
                sigset_t sigset;
                sigemptyset(&sigset);
                sigaddset(&sigset, static_cast<int>(Kind));
                if (sigprocmask(SIG_BLOCK, &sigset, NULL) != 0) [[unlikely]] {
                    LOG_ERROR("block signal {} failed", kind_to_string(Kind));
                    return;
                }
            }

            auto lock = std::lock_guard{s_mutex};
            s_handles.push_back(handle);

            if (runtime::detail::is_current_thread()) {
                sigset_t sigset;
                sigemptyset(&sigset);
                sigaddset(&sigset, static_cast<int>(Kind));
                if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) != 0) [[unlikely]] {
                    LOG_ERROR("unblock signal {} failed", kind_to_string(Kind));
                    return;
                }
            }
        }

    private:
        static void notify_all([[maybe_unused]] int code) {
            assert(code == static_cast<int>(Kind));

            auto handles = std::list<std::coroutine_handle<>>{};
            {
                auto lock = std::lock_guard{s_mutex};
                s_handles.swap(handles);
            }
            runtime::detail::schedule_remote_batch(std::move(handles), handles.size());
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
