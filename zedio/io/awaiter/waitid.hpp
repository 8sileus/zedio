#pragma once

#include "zedio/io/base/registrator.hpp"

namespace zedio::io {

class WaitId : public detail::IORegistrator<WaitId> {
private:
    using Super = detail::IORegistrator<WaitId>;

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

} // namespace zedio::io
