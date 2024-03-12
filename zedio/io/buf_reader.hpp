#pragma once

#include "zedio/io/buf/reader.hpp"

namespace zedio::io {

template <typename IO>
class BufReader : public detail::Reader<IO> {
public:
    BufReader(IO &&io, std::size_t size = detail::Config::DEFAULT_BUF_SIZE)
        : detail::Reader<IO>{io_, stream_}
        , io_{std::move(io)}
        , stream_{size} {}

    BufReader(BufReader &&other)
        : detail::Reader<IO>(io_, stream_)
        , io_{std::move(other.io_)}
        , stream_{std::move(other.stream_)} {}

    auto operator=(BufReader &&other) -> BufReader & {
        io_ = std::move(other.io_);
        stream_ = std::move(other.stream_);
        return *this;
    }

private:
    IO                   io_;
    detail::StreamBuffer stream_;
};

} // namespace zedio::io
