#pragma once

#include "zedio/io/impl/impl_async_read.hpp"

namespace zedio::socket::detail {

template <class T>
using ImplStreamRead = io::detail::ImplAsyncRead<T>;

} // namespace zedio::socket::detail
