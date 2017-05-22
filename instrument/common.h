#pragma once

#include <string>

#include "spdlog/spdlog.h"

#define LOG_ERROR_IF(logger, condition, ...)         \
    do {                                             \
        if ((condition)) logger->error(__VA_ARGS__); \
    } while (0)

#define VERIFY(logger, condition, ...)     \
    do {                                   \
        if (!(condition)) {                \
            logger->critical(__VA_ARGS__); \
            abort();                       \
        }                                  \
    } while (0)

namespace microtrace {

inline char* string_arr(std::string& str) { return &str[0]; }

extern std::shared_ptr<spdlog::logger> console_log;
}
