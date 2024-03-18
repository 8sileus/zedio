#pragma once

#include "zedio/runtime/config.hpp"

namespace zedio::runtime::detail {

template <class R>
class DriverBuilder;

template <class R>
class SchedulerBuilder;

template <class R>
class Builder {
public:
    [[nodiscard]]
    auto build() -> R {
        return R{this->config_};
    }

    [[nodiscard]]
    auto driver() -> DriverBuilder<R> {
        return *static_cast<DriverBuilder<R> *>(this);
    }

    [[nodiscard]]
    auto scheduler() -> SchedulerBuilder<R> {
        return *static_cast<SchedulerBuilder<R> *>(this);
    }

protected:
    Config config_{};
};

template <class R>
class DriverBuilder : public Builder<R> {
public:
    [[nodiscard]]
    auto set_custom_flags(unsigned int flag) -> DriverBuilder & {
        this->config_.ring_flags_ |= flag;
        return *this;
    }

    [[nodiscard]]
    auto set_ring_entries(std::size_t ring_entries) -> DriverBuilder & {
        this->config_.ring_entries_ = ring_entries;
        return *this;
    }

    [[nodiscard]]
    auto set_submit_interval(uint32_t interval) -> DriverBuilder & {
        this->config_.submit_interval_ = interval;
        return *this;
    }
};

template <class R>
class SchedulerBuilder : public Builder<R> {
public:
    [[nodiscard]]
    auto set_num_workers(std::size_t num) -> SchedulerBuilder & {
        this->config_.num_workers_ = num;
        return *this;
    }

    [[nodiscard]]
    auto set_check_io_interval(uint32_t interval) -> SchedulerBuilder & {
        this->config_.check_io_interval_ = interval;
        return *this;
    }

    [[nodiscard]]
    auto set_check_gloabal_interval(uint32_t interval) -> SchedulerBuilder & {
        this->config_.check_global_interval_ = interval;
        return *this;
    }
};

} // namespace zedio::runtime::detail
