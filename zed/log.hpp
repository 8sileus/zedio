#pragma once

#include "zed/common/util/singleton.hpp"
#include "zed/log/manager.hpp"

namespace zed::log {

auto &console = util::Singleton<detail::ConsoleLogger>::instance();

} // namespace zed::log