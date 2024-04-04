// -DASIO_HAS_IO_URING=ON -DASIO_DISABLE_EPOLL=ON -luring

#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
// C++
#include <array>
#include <iostream>
#include <thread>
#include <vector>

#include "asio.hpp"

template <typename T>
using Task = async_simple::coro::Lazy<T>;

namespace {

// Copy from https://github.com/alibaba/async_simple

class io_context_pool {
public:
    explicit io_context_pool(std::size_t pool_size) {
        if (pool_size == 0) {
            pool_size = 1; // set default value as 1
        }

        for (std::size_t i = 0; i < pool_size; ++i) {
            io_context_ptr io_context{std::make_shared<asio::io_context>()};
            work_ptr       work{std::make_shared<asio::io_context::work>(*io_context)};
            io_contexts_.push_back(io_context);
            work_.push_back(work);
        }
    }

    void run() {
        std::vector<std::shared_ptr<std::thread>> threads;
        for (std::size_t i = 0; i < io_contexts_.size(); ++i) {
            threads.emplace_back(
                std::make_shared<std::thread>([](io_context_ptr ctx) { ctx->run(); },
                                              io_contexts_[i]));
        }

        for (std::size_t i = 0; i < threads.size(); ++i) {
            threads[i]->join();
        }
    }

    void stop() {
        work_.clear();

        for (std::size_t i = 0; i < io_contexts_.size(); ++i) {
            io_contexts_[i]->stop();
        }
    }

    [[nodiscard]]
    auto current_io_context() const -> size_t {
        return next_io_context_ - 1;
    }

    auto get_io_context() -> asio::io_context & {
        asio::io_context &io_context = *io_contexts_[next_io_context_];
        ++next_io_context_;
        if (next_io_context_ == io_contexts_.size()) {
            next_io_context_ = 0;
        }
        return io_context;
    }

private:
    using io_context_ptr = std::shared_ptr<asio::io_context>;
    using work_ptr = std::shared_ptr<asio::io_context::work>;

    std::vector<io_context_ptr> io_contexts_;
    std::vector<work_ptr>       work_;
    std::size_t                 next_io_context_{0};
};

class AsioExecutor : public async_simple::Executor {
public:
    explicit AsioExecutor(asio::io_context &io_context)
        : io_context_(io_context) {}

    auto schedule(Func func) -> bool override {
        asio::post(io_context_, std::move(func));
        return true;
    }

private:
    asio::io_context &io_context_;
};

template <typename T>
    requires(!std::is_reference_v<T>)
struct AsioCallbackAwaiter {
public:
    using CallbackFunction = std::function<void(std::coroutine_handle<>, std::function<void(T)>)>;

    explicit AsioCallbackAwaiter(CallbackFunction callback_function)
        : callback_function_(std::move(callback_function)) {}

    auto await_ready() noexcept -> bool {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        callback_function_(handle, [this](T t) { result_ = std::move(t); });
    }

    auto coAwait([[maybe_unused]] async_simple::Executor *executor) noexcept {
        return std::move(*this);
    }

    auto await_resume() noexcept -> T {
        return std::move(result_);
    }

private:
    CallbackFunction callback_function_;
    T                result_;
};

auto async_accept(asio::ip::tcp::acceptor &acceptor, asio::ip::tcp::socket &socket) noexcept
    -> Task<std::error_code> {
    co_return co_await AsioCallbackAwaiter<std::error_code>{
        [&](std::coroutine_handle<> handle, auto set_resume_value) {
            acceptor.async_accept(
                socket,
                [handle, set_resume_value = std::move(set_resume_value)](auto ec) mutable {
                    set_resume_value(std::move(ec));
                    handle.resume();
                });
        }};
}

template <typename Socket, typename AsioBuffer>
auto async_read_some(Socket &socket, AsioBuffer &&buffer) noexcept
    -> Task<std::pair<std::error_code, size_t>> {
    co_return co_await AsioCallbackAwaiter<std::pair<std::error_code, size_t>>{
        [&](std::coroutine_handle<> handle, auto set_resume_value) mutable {
            socket.async_read_some(
                std::forward<AsioBuffer>(buffer),
                [handle, set_resume_value = std::move(set_resume_value)](auto ec,
                                                                         auto size) mutable {
                    set_resume_value(std::make_pair(std::move(ec), size));
                    handle.resume();
                });
        }};
}

template <typename Socket, typename AsioBuffer>
auto async_read(Socket &socket, AsioBuffer &buffer) noexcept
    -> Task<std::pair<std::error_code, size_t>> {
    co_return co_await AsioCallbackAwaiter<std::pair<std::error_code, size_t>>{
        [&](std::coroutine_handle<> handle, auto set_resume_value) mutable {
            asio::async_read(
                socket,
                buffer,
                [handle, set_resume_value = std::move(set_resume_value)](auto ec,
                                                                         auto size) mutable {
                    set_resume_value(std::make_pair(std::move(ec), size));
                    handle.resume();
                });
        }};
}

template <typename Socket, typename AsioBuffer>
auto async_read_until(Socket &socket, AsioBuffer &buffer, asio::string_view delim) noexcept
    -> Task<std::pair<std::error_code, size_t>> {
    co_return co_await AsioCallbackAwaiter<std::pair<std::error_code, size_t>>{
        [&](std::coroutine_handle<> handle, auto set_resume_value) mutable {
            asio::async_read_until(
                socket,
                buffer,
                delim,
                [handle, set_resume_value = std::move(set_resume_value)](auto ec,
                                                                         auto size) mutable {
                    set_resume_value(std::make_pair(std::move(ec), size));
                    handle.resume();
                });
        }};
}

template <typename Socket, typename AsioBuffer>
auto async_write(Socket &socket, AsioBuffer &&buffer) noexcept
    -> Task<std::pair<std::error_code, size_t>> {
    co_return co_await AsioCallbackAwaiter<std::pair<std::error_code, size_t>>{
        [&](std::coroutine_handle<> handle, auto set_resume_value) mutable {
            asio::async_write(
                socket,
                std::forward<AsioBuffer>(buffer),
                [handle, set_resume_value = std::move(set_resume_value)](auto ec,
                                                                         auto size) mutable {
                    set_resume_value(std::make_pair(std::move(ec), size));
                    handle.resume();
                });
        }};
}

} // namespace

constexpr std::string_view response = R"(
HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 13

Hello, World!
)";

auto process(asio::ip::tcp::socket sock) -> Task<void> {
    constexpr size_t             max_length = 128;
    std::array<char, max_length> data{};
    while (true) {
        if (auto [error, length] = co_await async_read_some(sock, asio::buffer(data));
            error || length == 0) {
            break;
        }

        if (auto [error, length] = co_await async_write(sock, asio::buffer(response)); error) {
            break;
        }
    }

    std::error_code ec;
    sock.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    sock.close(ec);
}

auto main() -> int {
    io_context_pool pool{4};

    auto thread = std::thread([&] { pool.run(); });
    async_simple::coro::syncAwait([&]() -> Task<void> {
        auto                   &io_context = pool.get_io_context();
        uint16_t                port{55555};
        asio::ip::tcp::acceptor acceptor(io_context,
                                         asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
        asio::signal_set        signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) {
            std::error_code ec;
            acceptor.close(ec);
        });
        std::cout << "Listening on " << port << " ...\n";
        while (true) {
            auto                 &ctx = pool.get_io_context();
            asio::ip::tcp::socket socket(ctx);
            auto                  error = co_await async_accept(acceptor, socket);
            if (error) {
                std::cout << "Accept failed, error: " << error.message() << '\n';
                if (error == asio::error::operation_aborted) {
                    break;
                }
                continue;
            }
            auto executor = std::make_shared<AsioExecutor>(ctx);
            process(std::move(socket)).via(executor.get()).detach();
        }
    }());

    pool.stop();
    if (thread.joinable()) {
        thread.join();
    }
    return 0;
}
