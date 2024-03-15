#pragma once

#include "zedio/io/buf/reader.hpp"
#include "zedio/io/buf/writer.hpp"

namespace zedio::io {

template <typename IO>
class BufStream : public detail::Reader<BufStream<IO>>, public detail::Writer<BufStream<IO>> {
    friend class detail::Reader<BufStream<IO>>;
    friend class detail::Writer<BufStream<IO>>;

public:
    BufStream(IO        &&io,
              std::size_t r_size = detail::StreamBuffer::DEFAULT_BUF_SIZE,
              std::size_t w_size = detail::StreamBuffer::DEFAULT_BUF_SIZE)
        : io_{std::move(io)}
        , r_stream_{r_size}
        , w_stream_{w_size} {}

    BufStream(BufWriter &&other) noexcept
        : io_{std::move(other.io_)}
        , r_stream_{std::move(other.r_stream_)}
        , w_stream_{std::move(other.w_stream)} {}

    auto operator=(BufStream &&other) noexcept -> BufStream & {
        io_ = std::move(other.io_);
        stream_ = std::move(other.stream_);
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
