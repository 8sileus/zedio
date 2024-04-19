#pragma once

#include "zedio/runtime/config.hpp"
#include "zedio/runtime/current_thread/handle.hpp"
#include "zedio/runtime/multi_thread/handle.hpp"
#include "zedio/runtime/runtime.hpp"

// C++
#include <string>
#include <thread>

namespace zedio::runtime {

enum class Kind {
    CurrentThread,
    MultiThread,
};

template <typename T>
class Runtime;

template <Kind KIND = Kind::MultiThread>
class Builder {
private:
    Builder() {
        auto _ = set_thread_basename("ZEDIO-WORKER");
    }

public:
    [[nodiscard]]
    auto set_num_workers(std::size_t num_worker_threads) -> Builder & {
        config_.num_workers_ = num_worker_threads;
        return *this;
    }

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
        if constexpr (KIND == Kind::CurrentThread) {
            return Runtime<current_thread::Handle>{std::move(config_),
                                                   std::move(build_thread_name_func_)};
        } else {
            return Runtime<multi_thread::Handle>{
                std::move(config_),
                std::move(build_thread_name_func_),
            };
        }
    }

public:
    [[nodiscard]]
    static auto options() -> Builder {
        return Builder{};
    }

    [[nodiscard]]
    static auto default_create() {
        return Builder{}.build();
    }

private:
    detail::Config                          config_{};
    std::function<std::string(std::size_t)> build_thread_name_func_{};
};

using CurrentThreadBuilder = Builder<Kind::CurrentThread>;

using MultiThreadBuilder = Builder<Kind::MultiThread>;

} // namespace zedio::runtime
