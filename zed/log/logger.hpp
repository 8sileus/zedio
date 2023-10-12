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
#include <source_location>
#include <thread>

namespace zed::log::detail {

class FmtWithSourceLocation {
public:
    template <typename T>
        requires std::constructible_from<std::string_view, T>
    FmtWithSourceLocation(T &&fmt, std::source_location sl = std::source_location::current())
        : fmt_(std::forward<T>(fmt))
        , sl_(std::move(sl)) {}

    constexpr auto fmt() const -> const std::string_view & { return fmt_; }

    constexpr auto source_location() const -> const std::source_location & { return sl_; }

private:
    std::string_view     fmt_;
    std::source_location sl_;
};

class BaseLogger : util::Noncopyable {
public:
    virtual ~BaseLogger() = default;

    virtual void log(std::string &&msg, const std::string_view &color) = 0;

    void setLevel(LogLevel level) noexcept { level_ = level; }

    [[nodiscard]]
    auto getLevel() const noexcept -> LogLevel {
        return level_;
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
        if (level < level_) {
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
        const auto &fmt = fwsl.fmt();
        const auto &sl = fwsl.source_location();
        this->log(
            std::format("{}.{:03} {} {} {}:{} {}\n", t_time_buffer, cur_millisecond,
                        level_to_string(level), this_thread::get_tid(), sl.file_name(), sl.line(),
                        std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...))),
            level_to_color(level));
    }

private:
    LogLevel level_{LogLevel::DEBUG};
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
        : file_{base_name}
        , current_buffer_{new Buffer}
        , thread_{&FileLogger::work, this} {
        for (int i = 0; i < 2; ++i) {
            empty_buffers_.emplace_back(new Buffer);
        }
    }

    ~FileLogger() {
        ruinning_ = false;
        cond_.notify_one();
        thread_.join();
    }

    void log(std::string &&msg, [[maybe_unused]] const std::string_view &color) override {
        if (!ruinning_) [[unlikely]] {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (current_buffer_->writeable_bytes() > msg.size()) {
            current_buffer_->write(msg);
        } else {
            full_buffers_.push_back(std::move(current_buffer_));
            if (!empty_buffers_.empty()) {
                current_buffer_ = std::move(empty_buffers_.front());
                empty_buffers_.pop_front();
            } else {
                current_buffer_.reset(new Buffer);
            }
            current_buffer_->write(msg);
            cond_.notify_one();
        }
    }

    void set_max_file_size(off_t roll_size) noexcept { file_.set_max_file_size(roll_size); }

private:
    void work() {
        constexpr std::size_t max_buffer_list_size = 15;

        while (ruinning_) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (full_buffers_.empty()) {
                    cond_.wait_for(lock, std::chrono::seconds(3));
                }
                full_buffers_.push_back(std::move(current_buffer_));
                if (!empty_buffers_.empty()) {
                    current_buffer_ = std::move(empty_buffers_.front());
                    empty_buffers_.pop_front();
                } else {
                    current_buffer_.reset(new Buffer);
                }
            }

            if (full_buffers_.size() > max_buffer_list_size) {
                char buf[256];
                std::cerr << std::format("Dropped log messages {} larger buffers\n",
                                         full_buffers_.size() - 2);
                full_buffers_.resize(2);
            }

            for (auto &buffer : full_buffers_) {
                file_.write(buffer->data(), buffer->size());
                buffer->reset();
            }

            if (full_buffers_.size() > 2) {
                full_buffers_.resize(2);
            }
            file_.flush();
            empty_buffers_.splice(empty_buffers_.end(), full_buffers_);
        }

        if (!current_buffer_->empty()) {
            full_buffers_.emplace_back(std::move(current_buffer_));
        }
        for (auto &buffer : full_buffers_) {
            file_.write(buffer->data(), buffer->size());
        }
    }

private:
    using Buffer = LogBuffer<4000 * 1024>;
    using BufferPtr = std::unique_ptr<Buffer>;

    LogFile                 file_;
    BufferPtr               current_buffer_{};
    std::list<BufferPtr>    empty_buffers_{};
    std::list<BufferPtr>    full_buffers_{};
    std::mutex              mutex_{};
    std::condition_variable cond_{};
    std::thread             thread_{};
    std::atomic<bool>       ruinning_{true};
};

} // namespace zed::log::detail
