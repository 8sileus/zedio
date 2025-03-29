#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Rename : public IORegistrator<Rename> {
    private:
        using Super = IORegistrator<Rename>;

    public:
        Rename(int olddfd, const char *oldpath, int newdfd, const char *newpath, int flags)
            : Super{io_uring_prep_renameat, olddfd, oldpath, newdfd, newpath, flags} {}

        Rename(const char *oldpath, const char *newpath)
            : Super{io_uring_prep_rename, oldpath, newpath} {}

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
static inline auto
renameat(int olddfd, const char *oldpath, int newdfd, const char *newpath, int flags) {
    return detail::Rename{olddfd, oldpath, newdfd, newpath, flags};
}

[[REMEMBER_CO_AWAIT]]
static inline auto rename(const char *oldpath, const char *newpath) {
    return detail::Rename{oldpath, newpath};
}

} // namespace zedio::io
