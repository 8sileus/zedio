#ifndef ZED_SRC_LOG_LOGAPPENDER_HPP_
#define ZED_SRC_LOG_LOGAPPENDER_HPP_

#include "log/log_buffer.hpp"
#include "log/log_file.hpp"
#include "noncopyable.hpp"

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

namespace zed::log {

class LogAppender : util::Noncopyable {
public:
    virtual ~LogAppender() = default;

    virtual void log(const std::string& msg) = 0;
};

class StdoutLogAppender : public LogAppender {
public:
    void log(const std::string& msg) override;
};

class FileLogAppender : public LogAppender {
public:
    FileLogAppender(const std::string& base_name);

    ~FileLogAppender();

    void log(const std::string& msg) override;

private:
    void loopFunc();

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

}  // namespace zed::log

#endif  // ZED_SRC_LOG_LOGAPPENDER_HPP_