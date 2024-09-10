#pragma once

#ifdef __linux__

#include "zedio/runtime/io/linux/poller.hpp"
#include "zedio/runtime/io/linux/waker.hpp"

#elif _WIN32

#include "zedio/runtime/io/win/poller.hpp"
#include "zedio/runtime/io/win/waker.hpp"

#endif