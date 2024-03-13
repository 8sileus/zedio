#pragma once

#include "zedio/io/buf/reader.hpp"
#include "zedio/io/buf/writer.hpp"

namespace zedio::io {

template <typename IO>
class BufStream : public detail::Reader<IO>, public detail::Writer<IO> {
public:
    BufStream(IO        &&io,
              std::size_t r_size = detail::StreamBuffer::DEFAULT_BUF_SIZE,
              std::size_t w_size = detail::StreamBuffer::DEFAULT_BUF_SIZE)
        : detail::Reader<IO>(io_, r_stream_)
        , detail::Writer<IO>{io_, w_stream_}
        , io_{std::move(io)}
        , r_stream_{r_size}
        , w_stream_{w_size} {}

    BufStream(BufWriter &&other)
        : detail::Reader<IO>(io_, r_stream_)
        , detail::Writer<IO>(io_, w_stream_)
        , io_{std::move(other.io_)}
        , r_stream_{std::move(other.r_stream_)}
        , w_stream_{std::move(other.w_stream)} {}

    auto operator=(BufStream &&other) -> BufStream & {
        io_ = std::move(other.io_);
        stream_ = std::move(other.stream_);
        return *this;
    }

private:
    IO                   io_;
    detail::StreamBuffer r_stream_;
    detail::StreamBuffer w_stream_;
};

} // namespace zedio::io
