#pragma once

#include "zedio/common/def.hpp"

#ifdef __linux__

#include "zedio/io/awaiter/linux/accept.hpp"

#elif _WIN32

#include "zedio/io/awaiter/win/accept.hpp"
#include "zedio/io/awaiter/win/connect.hpp"
#include "zedio/io/awaiter/win/read_file.hpp"
#include "zedio/io/awaiter/win/recv.hpp"
#include "zedio/io/awaiter/win/recv_from.hpp"
#include "zedio/io/awaiter/win/send.hpp"
#include "zedio/io/awaiter/win/send_to.hpp"
#include "zedio/io/awaiter/win/shutdown.hpp"
#include "zedio/io/awaiter/win/write_file.hpp"

#endif