#pragma once

#include "zedio/io/buf/stream.hpp"
#include "zedio/io/io.hpp"

namespace zedio::io {

class Writer {
public:
public:
    // flush();

    // [[nodiscard]]
    // auto take_writer() -> IO {
    //     return std::move(io_);
    // }
private:
    detail::IO        io_;
    detail::BufStream stream_;
};

} // namespace zedio::io
