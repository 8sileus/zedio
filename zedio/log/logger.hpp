#pragma once

#include "zedio/common/util/noncopyable.hpp"
#include "zedio/common/util/thread.hpp"
#include "zedio/log/buffer.hpp"
#include "zedio/log/file.hpp"
#include "zedio/log/level.hpp"

// C++
#include <array>
#include <condition_variable>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <source_location>
#include <thread>

namespace zedio::log::detail {

class FmtWithSourceLocation {
public:
    template <typename T>
        requires std::constructible_from<std::string_view, T>
    FmtWithSourceLocation(T &&fmt, std::source_location sl = std::source_location::current())
        : fmt_(std::forward<T>(fmt))
        , sl_(sl) {}

    [[nodiscard]]
    constexpr auto fmt() const -> std::string_view {
        return fmt_;
    }

    [[nodiscard]]
    constexpr auto source_location() const -> const std::source_location & {
        return sl_;
    }

private:
    std::string_view     fmt_;
    std::source_location sl_;
};

struct LogRecord {
    const char      *datetime;
    int64_t          millisecond;
    Tid              thread_id;
    std::string_view thread_name;
    const char      *file_name;
    size_t           line;
    std::string      log;
};

template <class DeriverLogger>
class BaseLogger : util::Noncopyable {
public:
    void set_level(LogLevel level) noexcept {
        level_ = level;
    }

    [[nodiscard]]
    auto level() const noexcept -> LogLevel {
        return level_;
    }

    template <typename... Args>
    void trace(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::Trace>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::Debug>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::Info>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::Warn>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::Error>(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal(FmtWithSourceLocation fmt, Args &&...args) {
        format<LogLevel::Fatal>(fmt, std::forward<Args>(args)...);
    }

private:
    template <LogLevel LEVEL, typename... Args>
    void format(const FmtWithSourceLocation &fwsl, const Args &...args) {
        static thread_local std::array<char, 64> buffer{};
        static thread_local time_t               last_second{0};

        if (LEVEL < level_) {
            return;
        }

        const auto now_time_point = std::chrono::system_clock::now();
        const auto current_second = std::chrono::system_clock::to_time_t(now_time_point);
        const auto current_millisecond = std::chrono::duration_cast<std::chrono::milliseconds>(
                                             now_time_point.time_since_epoch())
                                             .count()
                                         % 1000;

        if (current_second != last_second) {
            constexpr auto format = "%Y-%m-%d %H:%M:%S";
            const auto     now_tm = std::localtime(&current_second);
            std::strftime(buffer.data(), buffer.size(), format, now_tm);
        }

        const auto &fmt = fwsl.fmt();
        const auto &sl = fwsl.source_location();
        static_cast<DeriverLogger *>(this)->template log<LEVEL>(
            LogRecord{buffer.data(),
                      current_millisecond,
                      util::get_tid(),
                      util::get_current_thread_name(),
                      sl.file_name(),
                      sl.line(),
                      std::vformat(fmt, std::make_format_args(args...))});
    }

private:
    LogLevel level_{LogLevel::Debug};
};

class ConsoleLogger : public BaseLogger<ConsoleLogger> {
public:
    template <LogLevel level>
    void log(const LogRecord &record) {
        std::cout << std::format("{}.{:03} [{}{}{}] {} {} {}:{} {}\n",
                                 record.datetime,
                                 record.millisecond,
                                 level_to_color(level),
                                 level_to_string(level),
                                 reset_format(),
                                 record.thread_id,
                                 record.thread_name,
                                 record.file_name,
                                 record.line,
                                 record.log);
    }
};

class FileLogger : public BaseLogger<FileLogger> {
public:
    explicit FileLogger(std::string_view file_base_name)
        : file_{file_base_name}
        , current_buffer_{std::make_unique<Buffer>()}
        , thread_{&FileLogger::work, this} {
        for (int i = 0; i < 2; ++i) {
            empty_buffers_.emplace_back(std::make_unique<Buffer>());
        }
    }

    ~FileLogger() {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

    template <LogLevel level>
    void log(const LogRecord &record) {
        if (!running_) [[unlikely]] {
            return;
        }
        std::string msg{std::format("{}.{:03} {} {} {} {}:{} {}\n",
                                    record.datetime,
                                    record.millisecond,
                                    level_to_string(level),
                                    record.thread_id,
                                    record.thread_name,
                                    record.file_name,
                                    record.line,
                                    record.log)};

        std::lock_guard<std::mutex> lock(mutex_);
        if (current_buffer_->writable_bytes() > msg.size()) {
            current_buffer_->write(msg);
        } else {
            full_buffers_.push_back(std::move(current_buffer_));
            if (!empty_buffers_.empty()) {
                current_buffer_ = std::move(empty_buffers_.front());
                empty_buffers_.pop_front();
            } else {
                current_buffer_ = std::make_unique<Buffer>();
            }
            current_buffer_->write(msg);
            cond_.notify_one();
        }
    }

    void set_max_file_size(off_t roll_size) noexcept {
        file_.set_max_file_size(roll_size);
    }

private:
    void work() {
        constexpr std::size_t max_buffer_list_size = 15;

        while (running_) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait_for(lock, std::chrono::milliseconds(3), [this]() -> bool {
                    return !this->full_buffers_.empty();
                });
                // if (full_buffers_.empty()) {
                // cond_.wait_for(lock, std::chrono::seconds(3));
                // }
                full_buffers_.push_back(std::move(current_buffer_));
                if (!empty_buffers_.empty()) {
                    current_buffer_ = std::move(empty_buffers_.front());
                    empty_buffers_.pop_front();
                } else {
                    current_buffer_ = std::make_unique<Buffer>();
                }
            }

            if (full_buffers_.size() > max_buffer_list_size) {
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
        file_.flush();
    }

private:
    using Buffer = LogBuffer<4000 * 1024>;
    using BufferPtr = std::unique_ptr<Buffer>;

    LogFile                 file_;
    BufferPtr               current_buffer_;
    std::list<BufferPtr>    empty_buffers_{};
    std::list<BufferPtr>    full_buffers_{};
    std::mutex              mutex_{};
    std::condition_variable cond_{};
    std::thread             thread_{};
    std::atomic<bool>       running_{true};
};

} // namespace zedio::log::detail
