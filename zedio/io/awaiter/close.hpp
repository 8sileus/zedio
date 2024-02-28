#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Close : public detail::IORegistrator<Close, decltype(io_uring_prep_close)> {
private:
    using Super = detail::IORegistrator<Close, decltype(io_uring_prep_close)>;

public:
    Close(int fd)
        : Super{io_uring_prep_close, fd} {}

    auto await_resume() const noexcept -> Result<void> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
