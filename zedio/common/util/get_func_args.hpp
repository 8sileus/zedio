#pragma once

#include <tuple>

namespace zedio::util {

template <typename F>
struct GetFuncArgs;

template <typename R, typename... Args>
struct GetFuncArgs<R(Args...)> {
    using ReturnType = R;
    using TupleArgs = std::tuple<Args...>;
};

} // namespace zedio::util
