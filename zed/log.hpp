#pragma once

#include "zed/common/util/singleton.hpp"
#include "zed/log/manager.hpp"

namespace zed::log {

auto &console = util::Singleton<detail::ConsoleLogger>::instance();

using detail::LogLevel;

} // namespace zed::log