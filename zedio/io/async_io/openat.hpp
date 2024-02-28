#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class OpenAt : public detail::IORegistrator<OpenAt, decltype(io_uring_prep_openat)> {
private:
    using Super = detail::IORegistrator<OpenAt, decltype(io_uring_prep_openat)>;

public:
    OpenAt(int dfd, const char *path, int flags, mode_t mode)
        : Super{io_uring_prep_openat, dfd, path, flags, mode} {}

    auto await_resume() const noexcept -> Result<int> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return this->cb_.result_;
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
