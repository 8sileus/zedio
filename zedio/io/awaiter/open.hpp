#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Open : public detail::IORegistrator<Open> {
private:
    using Super = detail::IORegistrator<Open>;

public:
    Open(int dfd, const char *path, int flags, mode_t mode)
        : Super{io_uring_prep_openat, dfd, path, flags, mode} {}

    Open(const char *path, int flags, mode_t mode)
        : Open{AT_FDCWD, path, flags, mode} {}

    auto await_resume() const noexcept -> Result<int> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return this->cb_.result_;
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

class Open2 : public detail::IORegistrator<Open2> {
private:
    using Super = detail::IORegistrator<Open2>;

public:
    Open2(int dfd, const char *path, struct open_how *how)
        : Super{io_uring_prep_openat2, dfd, path, how} {}

    Open2(const char *path, struct open_how *how)
        : Open2{AT_FDCWD, path, how} {}

    auto await_resume() const noexcept -> Result<int> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return this->cb_.result_;
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
