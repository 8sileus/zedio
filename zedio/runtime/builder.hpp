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
        return R{std::move(config_)};
    }

    [[nodiscard]]
    auto driver() -> DriverBuilder<R> {
        return DriverBuilder<R>{config_};
    }

    [[nodiscard]]
    auto scheduler() -> SchedulerBuilder<R> {
        return SchedulerBuilder<R>{config_};
    }

private:
    Config config_;
};

template <class R>
class DriverBuilder : public Builder<R> {
public:
    DriverBuilder(Config &config)
        : config_{config} {}

public:
    [[nodiscard]]
    auto set_custom_flags(unsigned int flag) -> DriverBuilder & {
        config_.ring_flags_ |= flag;
        return *this;
    }

    [[nodiscard]]
    auto set_ring_entries(std::size_t ring_entries) -> DriverBuilder & {
        config_.ring_entries_ = ring_entries;
        return *this;
    }

    [[nodiscard]]
    auto set_submit_interval(uint32_t interval) -> DriverBuilder & {
        config_.submit_interval_ = interval;
        return *this;
    }

private:
    Config &config_;
};

template <class R>
class SchedulerBuilder : public Builder<R> {
public:
    SchedulerBuilder(Config &config)
        : config_{config} {}

public:
    [[nodiscard]]
    auto set_num_workers(std::size_t num) -> SchedulerBuilder & {
        config_.num_workers_ = num;
        return *this;
    }

    [[nodiscard]]
    auto set_check_io_interval(uint32_t interval) -> SchedulerBuilder & {
        config_.check_io_interval_ = interval;
        return *this;
    }

    [[nodiscard]]
    auto set_check_gloabal_interval(uint32_t interval) -> SchedulerBuilder & {
        config_.check_global_interval_ = interval;
        return *this;
    }

private:
    Config &config_;
};

} // namespace zedio::runtime::detail
