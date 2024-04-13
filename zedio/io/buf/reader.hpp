#pragma once

#include "zedio/io/impl/impl_buf_read.hpp"

namespace zedio::io {

template <class IO>
    requires requires(IO io, std::span<char> buf) {
        { io.read(buf) };
        { io.read_vectored(buf) };
    }
class BufReader : public detail::ImplBufRead<BufReader<IO>> {
private:
    friend class detail::ImplBufRead<BufReader<IO>>;

public:
    BufReader(IO &&io, std::size_t size = detail::StreamBuffer::DEFAULT_BUF_SIZE)
        : io_{std::move(io)}
        , r_stream_{size} {}

    BufReader(BufReader &&other) noexcept
        : io_{std::move(other.io_)}
        , r_stream_{std::move(other.r_stream_)} {}

    auto operator=(BufReader &&other) noexcept -> BufReader & {
        io_ = std::move(other.io_);
        r_stream_ = std::move(other.r_stream_);
        return *this;
    }

public:
    [[nodiscard]]
    auto inner() noexcept -> IO & {
        return io_;
    }

    [[nodiscard]]
    auto into_inner() noexcept -> IO {
        r_stream_.disable();
        return std::move(io_);
    }

private:
    IO                   io_;
    detail::StreamBuffer r_stream_;
};

} // namespace zedio::io
