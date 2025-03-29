#pragma once

#ifdef __linux__

#include "zedio/io/base/linux/io_request.hpp"

#elif _WIN32

#include "zedio/io/base/win/io_request.hpp"

#endif