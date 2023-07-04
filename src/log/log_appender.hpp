#pragma once

#include "log/log_buffer.hpp"
#include "log/log_file.hpp"
#include "noncopyable.hpp"

#include <condition_variable>
#include <format>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

namespace zed {

class [[nodiscard]] LogAppender : util::Noncopyable {
public:
    virtual ~LogAppender() = default;

    virtual void log(const std::string& msg) = 0;
};

class [[nodiscard]] StdoutLogAppender : public LogAppender {
public:
    void log(const std::string& msg) override {
        std::cout << std::format("{}", msg);
    }
};

class [[nodiscard]] FileLogAppender : public LogAppender {
public:
    FileLogAppender(const std::string_view& base_name)
        : m_file{base_name},
          m_current_buffer{new Buffer},
          m_thread{&FileLogAppender::loopFunc, this} {
        for (int i = 0; i < 2; ++i) {
            m_empty_buffers.emplace_back(new Buffer);
        }
    }

    ~FileLogAppender() {
        m_running = false;
        m_cond.notify_one();
        m_thread.join();
    }

    void log(const std::string& msg) override {
        if (!m_running) [[unlikely]] {
            return;
        }

        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        if (m_current_buffer->writableBytes() > msg.size()) {
            m_current_buffer->write(msg);
        } else {
            m_full_buffers.push_back(std::move(m_current_buffer));
            if (!m_empty_buffers.empty()) {
                m_current_buffer = std::move(m_empty_buffers.front());
                m_empty_buffers.pop_front();
            } else {
                m_current_buffer.reset(new Buffer);
            }
            m_current_buffer->write(msg);
            m_cond.notify_one();
        }
    }

private:
    void loopFunc() {
        constexpr std::size_t max_buffer_list_size = 15;

        while (m_running) {
            {
                std::unique_lock<std::mutex> lock(m_buffer_mutex);
                if (m_full_buffers.empty()) {
                    m_cond.wait_for(lock, std::chrono::seconds(3));
                }
                m_full_buffers.push_back(std::move(m_current_buffer));
                if (!m_empty_buffers.empty()) {
                    m_current_buffer = std::move(m_empty_buffers.front());
                    m_empty_buffers.pop_front();
                } else {
                    m_current_buffer.reset(new Buffer);
                }
            }

            if (m_full_buffers.size() > max_buffer_list_size) {
                char buf[256];
                std::cerr << std::format(
                    "Dropped log messages {} larger buffers\n",
                    m_full_buffers.size() - 2);
                m_full_buffers.resize(2);
            }

            for (auto& buffer : m_full_buffers) {
                m_file.write(buffer->data(), buffer->size());
                buffer->reset();
            }

            if (m_full_buffers.size() > 2) {
                m_full_buffers.resize(2);
            }

            m_file.flush();
            m_empty_buffers.splice(m_empty_buffers.end(), m_full_buffers);
        }

        if (!m_current_buffer->empty()) {
            m_full_buffers.emplace_back(std::move(m_current_buffer));
        }
        for (auto& buffer : m_full_buffers) {
            m_file.write(buffer->data(), buffer->size());
        }
    }

private:
    using Buffer = detail::LogBuffer<4000 * 1024>;
    using BufferPtr = std::unique_ptr<Buffer>;

    detail::LogFile         m_file;
    BufferPtr               m_current_buffer{};
    std::list<BufferPtr>    m_empty_buffers{};
    std::list<BufferPtr>    m_full_buffers{};
    std::mutex              m_buffer_mutex{};
    std::condition_variable m_cond{};
    std::thread             m_thread{};
    std::atomic<bool>       m_running{true};
};

}  // namespace zed
