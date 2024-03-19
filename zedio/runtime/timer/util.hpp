#pragma once

#include "zedio/runtime/timer/wheel.hpp"
// C
#include <cinttypes>
// C++
#include <memory>
#include <utility>
#include <variant>

namespace zedio::runtime::detail {

template <std::size_t... N>
static inline constexpr auto variant_wheel_helper_impl(std::index_sequence<N...>) {
    return std::variant<std::monostate, std::unique_ptr<Wheel<N>>...>{};
}

template <std::size_t N>
struct VariantWheelBuilder {
    using Type = decltype(variant_wheel_helper_impl(std::make_index_sequence<N>{}));
};

} // namespace zedio::runtime::detail
