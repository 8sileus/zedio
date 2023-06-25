#ifndef ZED_SRC_LOG_LOGFILE_HPP_
#define ZED_SRC_LOG_LOGFILE_HPP_

#include <cstdio>
#include <memory>
#include <string>

#include "noncopyable.hpp"

namespace zed::log::detail {
class LogFile : util::Noncopyable {
public:
    LogFile(const std::string& base_name) : m_base_name(base_name) {
        roll();
        setbuffer(m_file, m_buffer, sizeof(m_buffer));
    };

    ~LogFile() { fclose(m_file); }

    void append(const char* data, size_t len);

    void flush() { fflush(m_file); }

    void setFlushInterval(int interval) noexcept { m_flush_interval = interval; }

    void setCheckN(int check_n) noexcept { m_check_every_n = check_n; }

    void setRollSize(int roll_size) noexcept { m_roll_size = roll_size; }

private:
    void roll();

    static std::string GetLogFileName(const std::string& base_name);

private:
    constexpr static int kRollPerSeconds{60 * 60 * 24};
    const static int     kBufferSize{64 * 1024};

    const std::string m_base_name;
    off_t             m_roll_size{10 * 1024 * 1024};
    off_t             m_written_bytes{0};
    int               m_flush_interval{3};
    int               m_check_every_n{1024};
    int               m_count{0};
    FILE*             m_file{};
    char              m_buffer[kBufferSize];

    time_t            m_last_day{0};
    time_t            m_last_roll_time{0};
    time_t            m_last_flush_time{0};
};

}  // namespace zed::log::detail

#endif  // ZED_SRC_LOG_LOGFILE_HPP_