#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Open : public IORegistrator<Open> {
    private:
        using Super = IORegistrator<Open>;

    public:
        Open(int dfd, const char *path, int flags, mode_t mode)
            : Super{io_uring_prep_openat, dfd, path, flags, mode} {}

        Open(const char *path, int flags, mode_t mode)
            : Open{AT_FDCWD, path, flags, mode} {}

        auto await_resume() const noexcept -> Result<int> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return this->cb_.result_;
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

    class Open2 : public IORegistrator<Open2> {
    private:
        using Super = IORegistrator<Open2>;

    public:
        Open2(int dfd, const char *path, struct open_how *how)
            : Super{io_uring_prep_openat2, dfd, path, how} {}

        Open2(const char *path, struct open_how *how)
            : Open2{AT_FDCWD, path, how} {}

        auto await_resume() const noexcept -> Result<int> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return this->cb_.result_;
            } else {
                return ::std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
auto open(const char *path, int flags, mode_t mode) {
    return detail::Open{path, flags, mode};
}

[[REMEMBER_CO_AWAIT]]
static inline auto open2(const char *path, struct open_how *how) {
    return detail::Open2{path, how};
}

[[REMEMBER_CO_AWAIT]]
static inline auto openat(int dfd, const char *path, int flags, mode_t mode) {
    return detail::Open{dfd, path, flags, mode};
}

[[REMEMBER_CO_AWAIT]]
static inline auto openat2(int dfd, const char *path, struct open_how *how) {
    return detail::Open2{dfd, path, how};
}

} // namespace zedio::io
