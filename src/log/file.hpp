#pragma once

#include "util/noncopyable.hpp"

#include <cstdio>
#include <ctime>
#include <string>
#include <string_view>

namespace zed::log::detail {

class LogFile : util::Noncopyable {
public:
    LogFile(const std::string_view &base_name) : m_base_name{base_name} {
        roll();
        setbuffer(m_file, m_buffer, sizeof(m_buffer));
    }

    ~LogFile() { ::fclose(m_file); }

    void write(const char *data, size_t len) {
        off_t written = 0;
        while (written < len) {
            off_t remaining = len - written;
            off_t n = ::fwrite_unlocked(data + written, 1, remaining, m_file);
            written += n;
        }

        m_written_bytes += written;
        if (m_written_bytes > m_roll_size) {
            roll();
        }
    }

    void flush() { ::fflush(m_file); }

    void setRollSize(off_t roll_size) noexcept { m_roll_size = roll_size; }

private:
    void roll() {
        time_t now = ::time(nullptr);
        time_t current_day = now / PER_DAY_SECONDS;
        if (now > m_last_roll_time) {
            struct tm tm_time;
            ::localtime_r(&now, &tm_time);

            m_last_roll_day = current_day;
            m_last_roll_time = now;
            m_written_bytes = 0;
            if (m_file != nullptr) {
                ::fclose(m_file);
            }

            std::string file_name{m_base_name};
            file_name.reserve(file_name.size() + 64);
            char buf[32];
            ::strftime(buf, sizeof(buf), "-%Y%m%d-%H%M%S.log", &tm_time);
            file_name.append(buf);
            m_file = ::fopen(file_name.c_str(), "ae");
        }
    }

private:
    static constexpr std::size_t PER_DAY_SECONDS{60 * 60 * 24};
    static constexpr std::size_t BUFFER_SIZE{64 * 1024};

private:
    const std::string m_base_name;
    off_t             m_roll_size{10 * 1024 * 1024};
    off_t             m_written_bytes{0};
    int               m_count{0};
    FILE             *m_file{nullptr};
    char              m_buffer[BUFFER_SIZE];
    time_t            m_last_roll_day{0};
    time_t            m_last_roll_time{0};
};

} // namespace zed::log::detail
