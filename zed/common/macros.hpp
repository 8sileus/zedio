#pragma once

// C
#include <cstring>
// C++
#include <format>

namespace zed {

#define FORMAT_FUNC_ERR_MESSAGE(func_name, errno) \
    (std::format("call {} failed. errno: {}, message: {}", #func_name, errno, strerror(errno)))

} // namespace zed
