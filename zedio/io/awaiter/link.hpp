#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Link : public detail::IORegistrator<Link, decltype(io_uring_prep_linkat)> {
private:
    using Super = detail::IORegistrator<Link, decltype(io_uring_prep_linkat)>;

public:
    Link(int olddfd, const char *oldpath, int newdfd, const char *newpath, int flags)
        : Super{io_uring_prep_linkat, olddfd, oldpath, newdfd, newpath, flags} {}

    Link(const char *oldpath, const char *newpath, int flags)
        : Link{AT_FDCWD, oldpath, AT_FDCWD, newpath, flags} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
