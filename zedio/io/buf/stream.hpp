#pragma once

#include "zedio/io/impl/impl_buf_read.hpp"
#include "zedio/io/impl/impl_buf_write.hpp"

namespace zedio::io {

template <typename IO>
    requires requires(IO io, std::span<char> buf) {
        { io.read(buf) };
        { io.read_vectored(buf) };
        { io.write(buf) };
        { io.write_vectored(buf) };
    }
class BufStream : public detail::ImplBufRead<BufStream<IO>>,
                  public detail::ImplBufWrite<BufStream<IO>> {
    friend class detail::ImplBufRead<BufStream<IO>>;
    friend class detail::ImplBufWrite<BufStream<IO>>;

public:
    BufStream(IO        &&io,
              std::size_t r_size = detail::StreamBuffer::DEFAULT_BUF_SIZE,
              std::size_t w_size = detail::StreamBuffer::DEFAULT_BUF_SIZE)
        : io_{std::move(io)}
        , r_stream_{r_size}
        , w_stream_{w_size} {}

    BufStream(BufStream &&other) noexcept
        : io_{std::move(other.io_)}
        , r_stream_{std::move(other.r_stream_)}
        , w_stream_{std::move(other.w_stream)} {}

    auto operator=(BufStream &&other) noexcept -> BufStream & {
        io_ = std::move(other.io_);
        r_stream_ = std::move(other.r_stream_);
        w_stream_ = std::move(other.w_stream_);
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
        w_stream_.disable();
        return std::move(io_);
    }

private:
    IO                   io_;
    detail::StreamBuffer r_stream_;
    detail::StreamBuffer w_stream_;
};

} // namespace zedio::io
