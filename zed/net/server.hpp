#pragma once

#include "async.hpp"
#include "common/macros.hpp"
#include "log.hpp"
#include "net/address.hpp"
#include "net/detail/tcp_buffer.hpp"
#include "net/tcp_listener.hpp"
#include "net/tcp_stream.hpp"

// C++
#include <functional>
#include <memory>
#include <optional>

namespace zed::net {

class Server : util::Noncopyable {
private:
    Server(TcpListener &&listener, std::size_t worker_num)
        : scheduler_{std::make_unique<async::Scheduler>(worker_num)} {
        scheduler_->submit(accept_stream(std::move(listener)));
    }

public:
    ~Server() { scheduler_->stop(); }

    Server(Server &&other)
        : scheduler_{std::move(other.scheduler_)}
        , cb_{std::move(other.cb_)} {}

    void start() { scheduler_->start(); }

public:
    [[nodiscard]]
    static auto bind(Address &address, std::size_t worker_num = std::thread::hardware_concurrency())
        -> std::optional<Server> {
        if (auto listener = TcpListener::bind(address); listener) {
            return Server{std::move(listener.value()), worker_num};
        }
        return std::nullopt;
    }

    [[nodiscard]]
    static auto bind(const std::vector<Address> &addresses,
                     std::size_t                 worker_num = std::thread::hardware_concurrency())
        -> std::optional<Server> {
        if (auto listener = TcpListener::bind(addresses); listener) {
            return Server{std::move(listener.value()), worker_num};
        }
        return std::nullopt;
    }

private:
    auto accept_stream(TcpListener &&listener) -> async::Task<void> {
        while (true) {
            if (auto fd = co_await listener.accept(); fd >= 0) {
                auto stream = TcpStream{fd};
                LOG_DEBUG("Accept a connection from [{}]", stream.get_peer_address().to_string());
                scheduler_->submit(handle_stream(std::move(stream)));
            }
        }
    }

    auto handle_stream(TcpStream &&stream) -> async::Task<void> {
        detail::TcpBuffer input_buffer;
        detail::TcpBuffer output_buffer;
        if (auto n
            = co_await stream.read(input_buffer.begin_write(), input_buffer.writeable_bytes());
            n <= 0) [[unlikely]] {
            // LOG_ERROR
        } else {
            input_buffer.has_written(n);
        }

        if (auto n
            = co_await stream.write(output_buffer.begin_read(), output_buffer.readable_bytes());
            n <= 0) [[unlikely]] {
            // LOG_ERROR
        } else {
            input_buffer.retrieve(n);
        }
    }

private:
    std::unique_ptr<async::Scheduler>             scheduler_;
    std::function<async::Task<void>(TcpStream &)> cb_;
};

} // namespace zed::net
