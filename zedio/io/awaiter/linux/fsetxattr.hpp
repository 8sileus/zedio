#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class FSetXattr : public IORegistrator<FSetXattr> {
    private:
        using Super = IORegistrator<FSetXattr>;

    public:
        FSetXattr(int fd, const char *name, const char *value, int flags, unsigned int len)
            : Super{io_uring_prep_fsetxattr, fd, name, value, flags, len} {}

        auto await_resume() const noexcept -> Result<std::size_t> {
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
fsetxattr(int fd, const char *name, const char *value, int flags, unsigned int len) {
    return detail::FSetXattr{fd, name, value, flags, len};
}

} // namespace zedio::io
