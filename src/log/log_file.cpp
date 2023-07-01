#include "log/log_file.hpp"

#include <ctime>
#include <format>

namespace zed::log::detail {

[[nodiscard]]
auto GetLogFileName(const std::string& base_name) -> std::string {
    std::string file_name{base_name};
    file_name.reserve(file_name.size() + 64);

    time_t now_time = ::time(nullptr);

    struct tm tm_time;
    ::localtime_r(&now_time, &tm_time);

    char buf[32];
    ::strftime(buf, sizeof(buf), "-%Y%m%d-%H%M%S.log", &tm_time);
    file_name.append(buf);
    return file_name;
}

void LogFile::append(const char* data, size_t len) {
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
            } else if (now - m_last_flush_time > m_flush_interval) {  // exceed flush interval
                m_last_flush_time = now;
                flush();
            }
        }
    }
}

void LogFile::roll() {
    std::string file_name = GetLogFileName(m_base_name);
    time_t      now = ::time(nullptr);
    time_t      current_day = now / kRollPerSeconds;
    if (now > m_last_roll_time) {
        m_last_day = current_day;
        m_last_flush_time = now;
        m_last_roll_time = now;
        m_written_bytes = 0;
        if(m_file!=nullptr){
            ::fclose(m_file);
        }
        m_file = ::fopen(file_name.c_str(), "ae");
    }
}

}  // namespace zed::log::detail