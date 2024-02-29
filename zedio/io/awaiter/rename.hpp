#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Rename : public detail::IORegistrator<Rename, decltype(io_uring_prep_renameat)> {
private:
    using Super = detail::IORegistrator<Rename, decltype(io_uring_prep_renameat)>;

public:
    Rename(int olddfd, const char *oldpath, int newdfd, const char *newpath, int flags)
        : Super{io_uring_prep_renameat, olddfd, oldpath, newdfd, newpath, flags} {}

    Rename(const char *oldpath, const char *newpath)
        : Rename{AT_FDCWD, oldpath, AT_FDCWD, newpath, 0} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
