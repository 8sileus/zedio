#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class FAdvise : public detail::IORegistrator<FAdvise> {
private:
    using Super = detail::IORegistrator<FAdvise>;

public:
    FAdvise(int fd, __u64 offset, off_t len, int advice)
        : Super{io_uring_prep_fadvise, fd, offset, len, advice} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
