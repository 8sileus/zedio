#pragma once

#include "log/buffer.hpp"
#include "log/def.hpp"
#include "log/file.hpp"
#include "util/noncopyable.hpp"
#include "util/thread.hpp"

// C++
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

    [[nodiscard]]
    auto getLevel() const noexcept -> LogLevel {
        return m_level;
    }

    template <typename... Args>
    void trace(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::TRACE>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::DEBUG>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::INFO>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::WARN>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::ERROR>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::FATAL>(fmt, std::forward<Args>(args)...);
    }

private:
    template <LogLevel level, typename... Args>
    void format(const FmtWithSourceLocation &fwsl, Args &&...args) {
        if (level < m_level) {
            return;
        }

        timeval tv_time;
        ::gettimeofday(&tv_time, nullptr);
        auto cur_second = tv_time.tv_sec;
        auto cur_millisecond = tv_time.tv_usec / 1000;
        if (cur_second != t_last_second) {
            struct tm tm_time;
            ::localtime_r(&cur_second, &tm_time);
            const char *format = "%Y-%m-%d %H:%M:%S";
            ::strftime(t_time_buffer, sizeof(t_time_buffer), format, &tm_time);
        }
        const auto &fmt = fwsl.format();
        const auto &sl = fwsl.source_location();
        this->log(
            std::format("{}.{:03} {} {} {}:{} {}\n", t_time_buffer, cur_millisecond,
                        level_to_string(level), this_thread::get_tid(), sl.file_name(), sl.line(),
                        std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...))),
            level_to_color(level));
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
        : m_file{base_name}
        , m_current_buffer{new Buffer}
        , m_thread{&FileLogger::loopFunc, this} {
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
