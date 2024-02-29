#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Statx : public detail::IORegistrator<Statx, decltype(io_uring_prep_statx)> {
private:
    using Super = detail::IORegistrator<Statx, decltype(io_uring_prep_statx)>;

public:
    Statx(int dfd, const char *path, int flags, unsigned mask, struct statx *statxbuf)
        : Super{io_uring_prep_statx, dfd, path, flags, mask, statxbuf} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
