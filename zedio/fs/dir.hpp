#pragma once

#include "zedio/fs/io.hpp"

namespace zedio::fs {

[[REMEMBER_CO_AWAIT]]
static inline auto create_dir(std::string_view dir_path, mode_t mode = 0666) {
    return io::mkdir(dir_path.data(), mode);
}

// TODO
// [[REMEMBER_CO_AWAIT]]
// static inline auto create_dir_all(std::string_view dir_path) {
// }

[[REMEMBER_CO_AWAIT]]
static inline auto remove_dir(std::string_view dir_path) {
    return io::unlink(dir_path.data(), AT_REMOVEDIR);
}

// TODO
//  [[REMEMBER_CO_AWAIT]]
//  static inline auto remove_dir_all(std::string_view dir_path) {}

} // namespace zedio::fs