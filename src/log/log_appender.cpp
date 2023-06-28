#include "log/log_appender.hpp"

#include <format>
#include <iostream>

namespace zed::log {

void StdoutLogAppender::log(const std::string& msg) { std::cout << std::vformat(msg, {}); }

FileLogAppender::FileLogAppender(const std::string& base_name)
    : m_file{base_name}, m_current_buffer{new Buffer}, m_thread{&FileLogAppender::loopFunc, this} {
    for (int i = 0; i < 2; ++i) {
        m_empty_buffers.emplace_back(new Buffer);
    }
}

FileLogAppender::~FileLogAppender() {
    m_running = false;
    m_cond.notify_one();
    m_thread.join();
}

void FileLogAppender::log(const std::string& msg) {
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

void FileLogAppender::loopFunc() {
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
            std::cerr << " Dropped log messages " << m_full_buffers.size() - 2 << " larger buffers\n ";
            m_full_buffers.resize(2);
        }

        for (auto& buffer : m_full_buffers) {
            m_file.append(buffer->data(), buffer->writtenBytes());
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
        m_file.append(buffer->data(), buffer->writtenBytes());
    }
}

}  // namespace zed::log