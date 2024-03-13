#pragma once

#include "zedio/io/buf/writer.hpp"

namespace zedio::io {

template <typename IO>
class BufWriter : public detail::Writer<IO> {
public:
    BufWriter(IO &&io, std::size_t size = detail::StreamBuffer::DEFAULT_BUF_SIZE)
        : detail::Writer<IO>{io_, stream_}
        , io_{std::move(io)}
        , stream_{size} {}

    BufWriter(BufWriter &&other)
        : detail::Writer<IO>(io_, stream_)
        , io_{std::move(other.io_)}
        , stream_{std::move(other.stream_)} {}

    auto operator=(BufWriter &&other) -> BufWriter & {
        io_ = std::move(other.io_);
        stream_ = std::move(other.stream_);
        return *this;
    }

private:
    IO                   io_;
    detail::StreamBuffer stream_;
};

} // namespace zedio::io
