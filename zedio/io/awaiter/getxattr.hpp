#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class Getxattr : public detail::IORegistrator<Getxattr> {
private:
    using Super = detail::IORegistrator<Getxattr>;

public:
    Getxattr(const char *name, char *value, const char *path, unsigned int len)
        : Super{io_uring_prep_getxattr, name, value, path, len} {}

    auto await_resume() const noexcept -> Result<std::size_t> {
        if (this->cb_.result_ >= 0) [[likely]] {
            return static_cast<std::size_t>(this->cb_.result_);
        } else {
            return std::unexpected{make_sys_error(-this->cb_.result_)};
        }
    }
};

} // namespace zedio::io
