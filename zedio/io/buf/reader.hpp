#pragma once

#include "zedio/io/buf/stream.hpp"
#include "zedio/io/io.hpp"
// C++
#include <span>
#include <vector>

namespace zedio::io {

class BufReader {
public:
    BufReader(detail::IO &&io, std::size_t size)
        : io_{std::move(io)}
        , stream_{size} {}

public:
    [[nodiscard]]
    auto buffer() {
        return stream_.read_splice();
    }

    [[nodiscard]]
    auto buffer() const {
        return stream_.read_splice();
    }

    [[nodiscard]]
    auto capacity() -> std::size_t {
        return stream_.capacity();
    }

    void consume(std::size_t n) {
        n = std::min(n, stream_.readable_bytes());
        stream_.increase_rpos(n);
    }

    [[REMEMBER_CO_AWAIT]]
    auto read(std::span<char> buf) {
        return real_read(buf, [this]() -> bool { !this->stream_.empty(); });
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_exact(std::span<char> buf) {
        return real_read(buf, [this, exact_bytes = buf.size_bytes()]() -> bool {
            return this->stream_.readable_bytes() >= exact_bytes;
        });
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_until(std::string &buf, char end_flag) -> zedio::async::Task<Result<std::size_t>> {
        while (true) {
            if (auto splice = stream_.find_flag_and_return_splice(end_flag); !splice.empty()) {
                buf.append(splice.begin(), splice.end());
                stream_.increase_rpos(splice.size_bytes());
                break;
            } else {
                buf.append(stream_.read_splice().begin(), stream_.read_splice().end());
                stream_.reset_pos();
            }

            auto ret = co_await io_.read(stream_.write_splice());
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            if (ret.value() == 0) {
                break;
            }
        }
        co_return buf.size();
    }

    [[REMEMBER_CO_AWAIT]]
    auto read_line(std::string &buf) {
        return read_until(buf, '\n');
    }

private:
    template <class Pred>
        requires std::is_invocable_v<Pred>
    auto real_read(std::span<char> buf, Pred pred) -> zedio::async::Task<Result<std::size_t>> {
        if (stream_.capacity() < buf.size_bytes()) {
            auto len = stream_.write_to(buf);
            buf = buf.subspan(len, buf.size_bytes() - len);
            co_return co_await io_.read(buf);
        }

        if (pred()) {
            co_return stream_.write_to(buf);
        }

        if (stream_.writable_bytes() < buf.size_bytes()) {
            stream_.move_to_front();
        }

        assert(stream_.writable_bytes() >= buf.size_bytes());

        while (true) {
            auto ret = co_await io_.read(stream_.write_splice());
            if (!ret) [[unlikely]] {
                co_return std::unexpected{ret.error()};
            }
            stream_.increase_wpos(ret.value());
            if (pred()) {
                co_return stream_.write_to(buf);
            }
        }
    }

private:
    detail::IO        io_;
    detail::BufStream stream_;
};

} // namespace zedio::io
