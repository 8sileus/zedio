#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

namespace detail {

    class WaitId : public IORegistrator<WaitId> {
    private:
        using Super = IORegistrator<WaitId>;

    public:
        WaitId(idtype_t idtype, id_t id, siginfo_t *infop, int options, unsigned int flags)
            : Super{io_uring_prep_waitid, idtype, id, infop, options, flags} {}

        auto await_resume() const noexcept -> Result<void> {
            if (this->cb_.result_ >= 0) [[likely]] {
                return {};
            } else {
                return std::unexpected{make_sys_error(-this->cb_.result_)};
            }
        }
    };

} // namespace detail

[[REMEMBER_CO_AWAIT]]
static inline auto
waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options, unsigned int flags) {
    return detail::WaitId{idtype, id, infop, options, flags};
}

} // namespace zedio::io
