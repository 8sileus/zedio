#pragma once

#include "zedio/async/coroutine/task.hpp"
#include "zedio/common/macros.hpp"
#include "zedio/common/util/singleton.hpp"
#include "zedio/runtime/runtime.hpp"
#include "zedio/signal/kind.hpp"

namespace zedio::signal {

namespace detail {

    template <SignalKind Kind>
    class Signal {
    private:
        class Awaiter {
        public:
            constexpr auto await_ready() const noexcept -> bool {
                return false;
            }
            constexpr void await_suspend(std::coroutine_handle<> handle) const noexcept {
                util::Singleton<SignalHandler<Kind>>::instance().push_back(handle);
            }

            constexpr void await_resume() const noexcept {}
        };

    public:
        [[REMEMBER_CO_AWAIT]]
        constexpr auto recv() -> Awaiter {
            return Awaiter{};
        }
    };

} // namespace detail

template <SignalKind Kind>
[[nodiscard]]
static inline auto signal() -> detail::Signal<Kind> {
    return detail::Signal<Kind>{};
}

[[REMEMBER_CO_AWAIT]]
static inline auto ctrl_c() {
    return signal<SignalKind::Interrupt>().recv();
}

} // namespace zedio::signal
