#pragma once

#include "zedio/runtime/config.hpp"
#include "zedio/runtime/current_thread/handle.hpp"
#include "zedio/runtime/multi_thread/handle.hpp"
#include "zedio/runtime/runtime.hpp"

// C++
#include <string>
#include <thread>

namespace zedio::runtime {

namespace detail {

    template <typename H>
    class Runtime;

    template <typename B, typename H>
    class Builder {
    private:
        Builder() {
            auto _ = set_thread_basename("ZEDIO-WORKER");
        }

    public:
        [[nodiscard]]
        auto set_global_queue_interval(uint32_t interval) -> Builder & {
            config_.global_queue_interval_ = interval;
            return *this;
        }

        [[nodiscard]]
        auto set_io_interval(uint32_t interval) -> Builder & {
            config_.io_interval_ = interval;
            return *this;
        }

        [[nodiscard]]
        auto set_submit_interval(std::size_t interval) {
            config_.submit_interval_ = interval;
            return *this;
        }

        [[nodiscard]]
        auto set_thread_name_fn(std::function<std::string(std::size_t)> &&func) -> Builder & {
            build_thread_name_func_ = std::move(func);
            return *this;
        }

        [[nodiscard]]
        auto set_thread_basename(std::string_view basename) -> Builder & {
            build_thread_name_func_ = [basename = basename](std::size_t index) -> std::string {
                return std::format("{}-{}", basename, index);
            };
            return *this;
        }

        [[nodiscard]]
        auto build() {
            return Runtime<H>{std::move(config_), std::move(build_thread_name_func_)};
        }

    public:
        [[nodiscard]]
        static auto options() -> B {
            return B{};
        }

        [[nodiscard]]
        static auto default_create() {
            return B{}.build();
        }

    protected:
        detail::Config                          config_{};
        std::function<std::string(std::size_t)> build_thread_name_func_{};
    };

} // namespace detail

class CurrentThreadBuilder : public detail::Builder<CurrentThreadBuilder, current_thread::Handle> {
};

class MultiThreadBuilder : public detail::Builder<MultiThreadBuilder, current_thread::Handle> {
public:
    [[nodiscard]]
    auto set_num_workers(std::size_t num_worker_threads) -> MultiThreadBuilder & {
        config_.num_workers_ = num_worker_threads;
        return *this;
    }
};

} // namespace zedio::runtime
