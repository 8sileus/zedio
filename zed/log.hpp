#pragma once

#include "common/util/singleton.hpp"
#include "log/manager.hpp"

namespace zed::log {
    
auto &console = util::Singleton<detail::ConsoleLogger>::Instance();

} // namespace zed::log