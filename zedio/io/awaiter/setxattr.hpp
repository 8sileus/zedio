#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class SetXattr : public IORegistrator<SetXattr> {
    private:
        using Super = IORegistrator<SetXattr>;

    public:
        SetXattr(const char *name, const char *value, const char *path, int flags, unsigned int len)
            : Super{io_uring_prep_setxattr, name, value, path, flags, len} {}

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
auto setxattr(const char *name, const char *value, const char *path, int flags, unsigned int len) {
    return detail::SetXattr{name, value, path, flags, len};
}

} // namespace zedio::io
