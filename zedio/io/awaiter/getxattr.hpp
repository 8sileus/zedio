#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class GetXattr : public detail::IORegistrator<GetXattr> {
    private:
        using Super = detail::IORegistrator<GetXattr>;

    public:
        GetXattr(const char *name, char *value, const char *path, unsigned int len)
            : Super{io_uring_prep_getxattr, name, value, path, len} {}

        auto await_resume() const noexcept -> Result<std::size_t> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return static_cast<std::size_t>(this->cb_.result_);
            } else {
                return std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
auto getxattr(const char *name, char *value, const char *path, unsigned int len) {
    return detail::GetXattr{name, value, path, len};
}

} // namespace zedio::io
