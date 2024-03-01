#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class FAllocate : public detail::IORegistrator<FAllocate> {
private:
    using Super = detail::IORegistrator<FAllocate>;

public:
    FAllocate(int fd, int mode, __u64 offset, __u64 len)
        : Super{io_uring_prep_fallocate, fd, mode, offset, len} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io