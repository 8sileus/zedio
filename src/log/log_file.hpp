#pragma once

#include <cstdio>
#include <ctime>
#include <memory>
#include <string>

#include "noncopyable.hpp"

namespace zed::detail {

class LogFile : util::Noncopyable {
public:
    LogFile(const std::string_view& base_name) : m_base_name(base_name) {
        roll();
        setbuffer(m_file, m_buffer, sizeof(m_buffer));
    }

    ~LogFile() { ::fclose(m_file); }

    void write(const char* data, size_t len) {
        size_t written = 0;
        while (written < len) {
            size_t remaining = len - written;
            size_t n = ::fwrite_unlocked(data + written, 1, remaining, m_file);
            written += n;
        }

        m_written_bytes += written;
        if (m_written_bytes > m_roll_size) {
            roll();
        } else {
            ++m_count;
            if (m_count > m_check_every_n) {
                m_count = 0;
                time_t now = ::time(nullptr);
                time_t current_day = now / kRollPerSeconds;
                if (current_day != m_last_day) {  // isn't same day
                    roll();
                } else if (now - m_last_flush_time >
                           m_flush_interval) {  // exceed flush interval
                    m_last_flush_time = now;
                    flush();
                }
            }
        }
    }

    void flush() { ::fflush(m_file); }

private:
    void roll() {
        time_t now = ::time(nullptr);
        time_t current_day = now / kRollPerSeconds;
        if (now > m_last_roll_time) {
            m_last_day = current_day;
            m_last_flush_time = now;
            m_last_roll_time = now;
            m_written_bytes = 0;
            if (m_file != nullptr) {
                ::fclose(m_file);
            }
            m_file = ::fopen(newFilename().c_str(), "ae");
        }
    }

    [[nodiscard]] auto newFilename() -> std::string {
        std::string file_name{m_base_name};
        file_name.reserve(file_name.size() + 64);

        time_t now_time = ::time(nullptr);

        struct tm tm_time;
        ::localtime_r(&now_time, &tm_time);

        char buf[32];
        ::strftime(buf, sizeof(buf), "-%Y%m%d-%H%M%S.log", &tm_time);
        file_name.append(buf);
        return file_name;
    }

private:
    constexpr static int kRollPerSeconds{60 * 60 * 24};
    constexpr static int kBufferSize{64 * 1024};

    const std::string m_base_name;
    off_t             m_roll_size{10 * 1024 * 1024};
    off_t             m_written_bytes{0};
    int               m_flush_interval{3};
    int               m_check_every_n{1024};
    int               m_count{0};
    FILE*             m_file{nullptr};
    char              m_buffer[kBufferSize];
    time_t            m_last_day{0};
    time_t            m_last_roll_time{0};
    time_t            m_last_flush_time{0};
};

}  // namespace zed::detail
