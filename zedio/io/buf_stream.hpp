#pragma once

#include "zedio/io/buf/reader.hpp"
#include "zedio/io/buf/writer.hpp"

namespace zedio::io {

template <typename IO>
class BufStream : public detail::Reader<IO>, public detail::Writer<IO> {
public:
    BufStream(IO &&io, std::size_t buf_size = detail::StreamBuffer::DEFAULT_BUF_SIZE)
        : detail::Reader<IO>{io, stream_}
        , detail::Writer<IO>{io, stream_}
        , io_{std::move(io)}
        , stream_{buf_size} {}

    BufStream(BufStream &&other) noexcept
        : io_{std::move(other.io_)}
        , stream_{other.stream_} {}

    auto operator=(BufStream &&other) noexcept -> BufStream & {
        io_ = std::move(other.io_);
        stream_ = std::move(other.stream_);
        return *this;
    }

private:
    IO                   io_;
    detail::StreamBuffer stream_;
};

} // namespace zedio::io
