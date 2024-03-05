#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class SymLink : public IORegistrator<SymLink> {
    private:
        using Super = IORegistrator<SymLink>;

    public:
        SymLink(const char *target, int newdirfd, const char *linkpath)
            : Super{io_uring_prep_symlinkat, target, newdirfd, linkpath} {}

        SymLink(const char *target, const char *linkpath)
            : Super{io_uring_prep_symlink, target, linkpath} {}

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
static inline auto symlink(const char *target, const char *linkpath) {
    return detail::SymLink{target, linkpath};
}

[[REMEMBER_CO_AWAIT]]
static inline auto symlinkat(const char *target, int newdirfd, const char *linkpath) {
    return detail::SymLink{target, newdirfd, linkpath};
}

} // namespace zedio::io
