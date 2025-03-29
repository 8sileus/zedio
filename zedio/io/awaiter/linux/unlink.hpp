#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Unlink : public IORegistrator<Unlink> {
    private:
        using Super = IORegistrator<Unlink>;

    public:
        Unlink(int dfd, const char *path, int flags)
            : Super{io_uring_prep_unlinkat, dfd, path, flags} {}

        Unlink(const char *path, int flags)
            : Super{io_uring_prep_unlink, path, flags} {}

        auto await_resume() const noexcept -> Result<void> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return {};
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto unlinkat(int dfd, const char *path, int flags) {
    return detail::Unlink(dfd, path, flags);
}

[[REMEMBER_CO_AWAIT]]
static inline auto unlink(const char *path, int flags) {
    return detail::Unlink{path, flags};
}

} // namespace zedio::io
