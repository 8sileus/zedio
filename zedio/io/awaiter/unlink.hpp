#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Unlink : public detail::IORegistrator<Unlink, decltype(io_uring_prep_unlinkat)> {
private:
    using Super = detail::IORegistrator<Unlink, decltype(io_uring_prep_unlinkat)>;

public:
    Unlink(int dfd, const char *path, int flags)
        : Super{io_uring_prep_unlinkat, dfd, path, flags} {}

    Unlink(const char *path, int flags)
        : Unlink{AT_FDCWD, path, flags} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
