#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class Statx : public IORegistrator<Statx> {
    private:
        using Super = IORegistrator<Statx>;

    public:
        Statx(int dfd, const char *path, int flags, unsigned mask, struct statx *statxbuf)
            : Super{io_uring_prep_statx, dfd, path, flags, mask, statxbuf} {}

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
auto statx(int dfd, const char *path, int flags, unsigned mask, struct statx *statxbuf) {
    return detail::Statx{dfd, path, flags, mask, statxbuf};
}

} // namespace zedio::io
