#pragma once

#include "common/util/singleton.hpp"
#include "log/manager.hpp"

namespace zed::log {

auto &console = util::Singleton<detail::ConsoleLogger>::Instance();

auto &zed_logger = console;

} // namespace zed::log