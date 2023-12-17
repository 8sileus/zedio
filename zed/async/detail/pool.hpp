#pragma once

#include "common/config.hpp"
// C
#include <cassert>
// C++
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <deque>
#include <expected>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace zed::async::detail {

class SpawnerMetrics {
public:
    auto num_threads() const -> uint { return num_threads_.load(std::memory_order_relaxed); }
    auto num_idle_threads() const -> uint {
        return num_idle_threads_.load(std::memory_order_relaxed);
    }
    auto queue_depth() const -> uint { return queue_depth_.load(std::memory_order_relaxed); }

    void inc_num_threads() { num_threads_.fetch_add(1, std::memory_order_relaxed); }
    void dec_num_threads() { num_threads_.fetch_sub(1, std::memory_order_relaxed); }
    void inc_num_idle_threads() { num_idle_threads_.fetch_add(1, std::memory_order_relaxed); }
    auto dec_num_idle_threads() -> uint {
        return num_idle_threads_.fetch_sub(1, std::memory_order_relaxed);
    }
    void inc_queue_depth() { queue_depth_.fetch_add(1, std::memory_order_relaxed); }
    void dec_queue_depth() { queue_depth_.fetch_sub(1, std::memory_order_relaxed); }

private:
    std::atomic<uint> num_threads_;
    std::atomic<uint> num_idle_threads_;
    std::atomic<uint> queue_depth_;
};

struct Shared {
    std::deque<std::coroutine_handle<>>          queue_;
    std::uint32_t                                num_notify_;
    bool                                         shutdown_;
    std::thread                                  last_exiting_thread_;
    std::unordered_map<std::size_t, std::thread> worker_threads_;
    std::size_t                                  worker_thread_index_;
};

struct Inner {
public:
    ~Inner() {
        cond_.notify_all();
        std::unique_lock lock(shared_mutex_);
        cond_.wait(lock);
        for (auto &[_, thread] : shared_.worker_threads_) {
            thread.join();
        }
    }

    auto spawn_coro(std::coroutine_handle<> coro) -> std::expected<void, std::error_code> {
        std::unique_lock lock(shared_mutex_);
        if (shared_.shutdown_) {
            // TODO throw error
        }
        shared_.queue_.push_back(coro);
        metrics_.inc_queue_depth();

        // No thread can process the coro immediately
        if (metrics_.num_idle_threads() == 0) {
            if (metrics_.num_threads() == thread_cap_) {
                // At max number of threads only push the coro
            } else {
                auto id = shared_.worker_thread_index_;
                try {
                    shared_.worker_threads_.emplace(id, std::thread{Inner::run, this, id});
                    shared_.worker_thread_index_ += 1;
                } catch (const std::exception &e) {
                    // return err;
                }
            }
        } else {
            metrics_.dec_num_idle_threads();
            shared_.num_notify_ += 1;
            cond_.notify_one();
        }
    }

private:
    void run(std::size_t worker_thread_id) {
        if (after_start_) {
            after_start_();
        }

        std::thread need_join_thread{};
        {
            std::unique_lock lock(shared_mutex_);
            while (true) {
                // { --BUSY--
                consume_queue_until_empty(lock);
                // } --BUSY--

                // { --IDLE--
                metrics_.inc_num_idle_threads();

                while (!shared_.shutdown_) {
                    auto status = cond_.wait_for(lock, keep_alive);
                    if (shared_.num_notify_ != 0) {
                        // receive a wakeup ,
                        // so change idle to busy to consume queue
                        shared_.num_notify_ -= 1;
                        break;
                    }
                    // If current thread is waiting for a wakeup timeout
                    // and thread pool is not shutdown
                    if (!shared_.shutdown_ & status == std::cv_status::timeout) {
                        // We get last exitting thread to release system resources
                        // and set current thread to last exitting thread
                        // next exitting thread will do the same thing
                        need_join_thread = std::move(shared_.last_exiting_thread_);
                        shared_.last_exiting_thread_
                            = std::move(shared_.worker_threads_[worker_thread_id]);
                        shared_.worker_threads_.erase(worker_thread_id);
                        goto loop_exit;
                    }
                }
                // } --IDLE--
                if (shared_.shutdown_) {
                    consume_queue_until_empty(lock);
                    metrics_.inc_num_idle_threads();
                    break;
                }
            }
        loop_exit:
            // Thread exit
            metrics_.dec_num_threads();

            auto prev_idle = metrics_.dec_num_idle_threads();
            assert(prev_idle >= metrics_.num_idle_threads());

            // When desctruct a Inner, the Inner will wait all thread exit
            // so the last thread need to wake up the Inner to continue desctructing
            if (shared_.shutdown_ && metrics_.num_threads() == 0) {
                cond_.notify_one();
            }
        }
        if (before_stop_) {
            before_stop_();
        }

        if (need_join_thread.joinable()) {
            need_join_thread.join();
        }
    }

    void consume_queue_until_empty(std::unique_lock<std::mutex> &lock) {
        while (!shared_.queue_.empty()) {
            auto coro = std::move(shared_.queue_.front());
            shared_.queue_.pop_front();
            metrics_.dec_queue_depth();
            lock.unlock();
            // TODO future: wrapping coro and set some status for coro
            coro.resume();
            lock.lock();
        }
    }

private:
    Shared                    shared_;
    std::mutex                shared_mutex_;
    std::condition_variable   cond_;
    std::string               name_;
    std::function<void()>     after_start_;
    std::function<void()>     before_stop_;
    std::size_t               thread_cap_;
    std::chrono::milliseconds keep_alive;
    SpawnerMetrics            metrics_;
};

} // namespace zed::async::detail
