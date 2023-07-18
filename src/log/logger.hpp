#pragma once

#include "log/buffer.hpp"
#include "log/event.hpp"
#include "log/file.hpp"
#include "util/noncopyable.hpp"

#include <condition_variable>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>

namespace zed::log::detail {

class BaseLogger : util::Noncopyable {
public:
    virtual ~BaseLogger() = default;

    virtual void log(std::string &&msg, const std::string_view &color) = 0;

    void setLevel(LogLevel level) noexcept { m_level = level; }

    [[nodiscard]] auto getLevel() const noexcept -> LogLevel { return m_level; }

    template <typename... Args>
    void trace(const std::string_view &fmt, Args &&...args) {
        format<LogLevel::TRACE>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(const std::string_view &fmt, Args &&...args) {
        format<LogLevel::DEBUG>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(const std::string_view &fmt, Args &&...args) {
        format<LogLevel::INFO>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(const std::string_view &fmt, Args &&...args) {
        format<LogLevel::WARN>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(const std::string_view &fmt, Args &&...args) {
        format<LogLevel::ERROR>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal(const std::string_view &fmt, Args &&...args) {
        format<LogLevel::FATAL>(fmt, std::forward<Args>(args)...);
    }

private:
    template <LogLevel level, typename... Args>
    void format(const std::string_view &fmt, Args &&...args) {
        if (level >= m_level) {
            auto cb = [this](std::string &&msg) { this->log(std::move(msg), LevelToColor(level)); };
            LogEvent<level>(std::move(cb)).format(fmt, std::forward<Args>(args)...);
        }
    }

private:
    LogLevel m_level{LogLevel::DEBUG};
};

class ConsoleLogger : public BaseLogger {
public:
    void log(std::string &&msg, const std::string_view &color) override {
        std::cout << std::format("{}{}", color, msg);
    }
};

class FileLogger : public BaseLogger {
public:
    FileLogger(const std::string_view &base_name)
        : m_file{base_name}, m_current_buffer{new Buffer}, m_thread{&FileLogger::loopFunc, this} {
        for (int i = 0; i < 2; ++i) {
            m_empty_buffers.emplace_back(new Buffer);
        }
    }

    ~FileLogger() {
        m_running = false;
        m_cond.notify_one();
        m_thread.join();
    }

    void log(std::string &&msg, [[maybe_unused]] const std::string_view &color) override {
        if (!m_running) [[unlikely]] {
            return;
        }
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        if (m_current_buffer->writeableSize() > msg.size()) {
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

    void setRollSize(off_t roll_size) noexcept { m_file.setRollSize(roll_size); }

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
                std::cerr << std::format("Dropped log messages {} larger buffers\n",
                                         m_full_buffers.size() - 2);
                m_full_buffers.resize(2);
            }

            for (auto &buffer : m_full_buffers) {
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
        for (auto &buffer : m_full_buffers) {
            m_file.write(buffer->data(), buffer->size());
        }
    }

private:
    using Buffer = LogBuffer<4000 * 1024>;
    using BufferPtr = std::unique_ptr<Buffer>;

    LogFile                 m_file;
    BufferPtr               m_current_buffer{};
    std::list<BufferPtr>    m_empty_buffers{};
    std::list<BufferPtr>    m_full_buffers{};
    std::mutex              m_buffer_mutex{};
    std::condition_variable m_cond{};
    std::thread             m_thread{};
    std::atomic<bool>       m_running{true};
};

} // namespace zed::log::detail
