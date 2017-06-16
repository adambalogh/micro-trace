#pragma once

#include <string>

#include "spdlog/spdlog.h"

#if BUILDING_LIBMICROTRACE && HAVE_VISIBILITY
#define LIBMICROTRACE_EXPORTED __attribute__((__visibility__("default")))
#else
#define LIBMICROTRACE_EXPORTED
#endif

#define LOG_ERROR_IF(condition, ...)                              \
    do {                                                          \
        if ((condition) == true) console_log->error(__VA_ARGS__); \
    } while (0)

#define VERIFY(condition, ...)                  \
    do {                                        \
        if ((condition) == false) {             \
            console_log->critical(__VA_ARGS__); \
            abort();                            \
        }                                       \
    } while (0)

#ifdef DEBUG
#define DLOG(msg, ...)        \
    printf(msg, __VA_ARGS__); \
    printf("\n");
#else
#define DLOG(msg, ...)
#endif

namespace microtrace {

inline char* string_arr(std::string& str) { return &str[0]; }

extern std::shared_ptr<spdlog::logger> console_log;
}
