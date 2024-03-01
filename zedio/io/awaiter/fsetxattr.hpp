#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class FSetXattr : public detail::IORegistrator<FSetXattr> {
private:
    using Super = detail::IORegistrator<FSetXattr>;

public:
    FSetXattr(int fd, const char *name, const char *value, int flags, unsigned int len)
        : Super{io_uring_prep_fsetxattr, fd, name, value, flags, len} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return {};
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
