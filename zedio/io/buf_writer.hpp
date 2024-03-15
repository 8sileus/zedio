#pragma once

#include "zedio/io/buf/writer.hpp"

namespace zedio::io {

template <class IO>
    requires requires(IO io, std::span<const char> buf) {
        { io.write(buf) };
    }
class BufWriter : public detail::Writer<BufWriter<IO>> {
private:
    friend class detail::Writer<BufWriter<IO>>;

public:
    BufWriter(IO &&io, std::size_t size = detail::StreamBuffer::DEFAULT_BUF_SIZE)
        : io_{std::move(io)}
        , w_stream_{size} {}

    BufWriter(BufWriter &&other) noexcept
        : io_{std::move(other.io_)}
        , w_stream_{std::move(other.w_stream_)} {}

    auto operator=(BufWriter &&other) noexcept -> BufWriter & {
        io_ = std::move(other.io_);
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
        w_stream_.disable();
        return std::move(io_);
    }

private:
    IO                   io_;
    detail::StreamBuffer w_stream_;
};

} // namespace zedio::io
