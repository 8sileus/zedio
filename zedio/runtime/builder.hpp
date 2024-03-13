#pragma once

#include "zedio/runtime/config.hpp"

namespace zedio::runtime::detail {

template <typename Runtime>
class Builder {
public:
    [[nodiscard]]
    auto set_num_workers(std::size_t num_workers) -> Builder & {
        config_.num_workers_ = num_workers;
        return *this;
    }

    [[nodiscard]]
    auto set_ring_entries(std::size_t ring_entries) -> Builder & {
        config_.ring_entries_ = ring_entries;
        return *this;
    }

    [[nodiscard]]
    auto set_check_io_interval(uint32_t interval) -> Builder & {
        config_.check_io_interval_ = interval;
        return *this;
    }

    [[nodiscard]]
    auto set_check_gloabal_interval(uint32_t interval) -> Builder & {
        config_.check_global_interval_ = interval;
        return *this;
    }

    [[nodiscard]]
    auto set_submit_interval(uint32_t interval) -> Builder & {
        config_.submit_interval_ = interval;
        return *this;
    }

    [[nodiscard]]
    auto set_custom_flags(unsigned int flag) -> Builder & {
        config_.ring_flags_ |= flag;
        return *this;
    }

    [[nodiscard]]
    auto build() -> Runtime {
        return Runtime{std::move(config_)};
    }

private:
    Config config_;
};

} // namespace zedio::runtime::detail
