#pragma once

#include "zedio/io/io.hpp"

namespace zedio::net::detail {

class SocketIO : public io::IO {
public:
    SocketIO(int fd)
        : IO{fd} {}

private:
    template <class T>
    [[nodiscard]]
    static auto open(const std::string_view &path, int flags, mode_t mode) {
        class Awaiter : public OpenAt {
        public:
            Awaiter(int fd, const std::string_view &path, int flags, mode_t mode)
                : OpenAt{fd, path.data(), flags, mode} {}

            auto await_resume() const noexcept -> Result<T> {
                if (this->cb_.result_ >= 0) [[likely]] {
                    return T{IO{this->cb_.result_}};
                } else {
                    return std::unexpected{make_sys_error(-this->cb_.result_)};
                }
            }
        };
        return Awaiter{AT_FDCWD, path, flags, mode};
    }
};

} // namespace zedio::net::detail
