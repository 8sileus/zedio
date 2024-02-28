#pragma once

#include "zedio/io/io.hpp"

namespace zedio::fs::detail {

class FileIO : public io::IO {
public:
    FileIO(int fd)
        : IO{fd} {}

private:
    [[nodiscard]]
    static auto socket(int domain, int type, int protocol) -> Result<IO> {
        if (auto fd = ::socket(domain, type | SOCK_NONBLOCK, protocol); fd != -1) [[likely]] {
            return SocketIO{fd};
        } else {
            return std::unexpected{make_sys_error(errno)};
        }
    }
};

} // namespace zedio::fs::detail
