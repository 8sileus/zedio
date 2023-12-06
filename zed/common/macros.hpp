#pragma once

// C
#include <cstring>
// C++
#include <format>

namespace zed {

// #ifdef ZED_LOG
#define LOG_TRACE(...) log::zed_logger.trace(__VA_ARGS__)
#define LOG_INFO(...) log::zed_logger.info(__VA_ARGS__)
#define LOG_DEBUG(...) log::zed_logger.debug(__VA_ARGS__)
#define LOG_WARN(...) log::zed_logger.warn(__VA_ARGS__)
#define LOG_ERROR(...) log::zed_logger.error(__VA_ARGS__)
#define LOG_FATAL(...) log::zed_logger.fatal(__VA_ARGS__)
// #else
// #define LOG_TRACE( ...)
// #define LOG_INFO(...)
// #define LOG_DEBUG( ...)
// #define LOG_WARN(...)
// #define LOG_ERROR(...)
// #define LOG_FATAL(...)
// #endif

#define FORMAT_FUNC_ERR_MESSAGE(func_name, err) \
    (std::format("Call {} failed. errno: {}, message: {}", #func_name, err, strerror(err)))

#define LOG_SYSERR(func_name, errno) \
    LOG_ERROR("Call {} failed. error : {}", #func_name, ::strerror(errno));

#define LOG_FD_SYSERR(fd, func_name, err) \
    LOG_ERROR("Call {} failed. error : {}", #func_name, fd, ::strerror(err));



} // namespace zed
