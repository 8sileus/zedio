#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Write : public detail::IORegistrator<Write> {
private:
    using Super = detail::IORegistrator<Write>;

public:
    Write(int fd, const void *buf, unsigned nbytes, __u64 offset)
        : Super{io_uring_prep_write, fd, buf, nbytes, offset} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return static_cast<std::size_t>(this->cb_.result_);
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
