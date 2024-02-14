#pragma once

#include "zedio/common/util/singleton.hpp"
#include "zedio/log/manager.hpp"

namespace zedio::log {

auto &console = util::Singleton<detail::ConsoleLogger>::instance();

using detail::LogLevel;

} // namespace zedio::log