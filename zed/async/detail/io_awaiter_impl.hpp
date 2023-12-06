#pragma once

#include "async/detail/io_awaiter.hpp"
#include "async/detail/processor.hpp"

namespace zed::async::detail {

void LazyBaseIOAwaiter::await_suspend(std::coroutine_handle<> handle) {
    handle_ = std::move(handle);
    auto sqe = io_uring_get_sqe(t_processor->get_uring());
    if (sqe == nullptr) [[unlikely]] {
        t_processor->push_awaiter(this);
    } else {
        cb_(sqe);
        io_uring_sqe_set_data64(sqe, reinterpret_cast<unsigned long long>(this));
        io_uring_submit(t_processor->get_uring());
    }
}

} // namespace zed::async::detail
