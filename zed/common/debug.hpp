#pragma once

#ifdef ZED_LOG

#include "log.hpp"

auto &debug_logger = zed::log::console;

#define SET_LOG_LEVEL(level) debug_logger.setLevel(level)

#define LOG_TRACE(...) debug_logger.trace(__VA_ARGS__)
#define LOG_INFO(...) debug_logger.info(__VA_ARGS__)
#define LOG_DEBUG(...) debug_logger.debug(__VA_ARGS__)
#define LOG_WARN(...) debug_logger.warn(__VA_ARGS__)
#define LOG_ERROR(...) debug_logger.error(__VA_ARGS__)
#define LOG_FATAL(...) debug_logger.fatal(__VA_ARGS__)

#else

#define SET_LOG_LEVEL(level) (level)

#define LOG_TRACE(...) (__VA_ARGS__)
#define LOG_INFO(...) (__VA_ARGS__)
#define LOG_DEBUG(...) (__VA_ARGS__)
#define LOG_WARN(...) (__VA_ARGS__)
#define LOG_ERROR(...) (__VA_ARGS__)
#define LOG_FATAL(...) (__VA_ARGS__)

#endif
