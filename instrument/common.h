#pragma once

#include <netinet/in.h>
#include <uv.h>
#include <string>

#include "spdlog/spdlog.h"

#if BUILDING_LIBMICROTRACE && HAVE_VISIBILITY
#define LIBMICROTRACE_EXPORTED __attribute__((__visibility__("default")))
#else
#define LIBMICROTRACE_EXPORTED
#endif

#define VERIFY(condition, format, ...)                                   \
    do {                                                                 \
        if ((condition) == false) {                                      \
            console_log->critical("{}:{} - " format, __FILE__, __LINE__, \
                                  ##__VA_ARGS__);                        \
            abort();                                                     \
        }                                                                \
    } while (0)

#ifdef DEBUG

#define LOG_ERROR_IF(condition, ...)                              \
    do {                                                          \
        if ((condition) == true) console_log->error(__VA_ARGS__); \
    } while (0)

#define DLOG(msg, ...)        \
    printf(msg, __VA_ARGS__); \
    printf("\n");

#else

#define DLOG(msg, ...)
#define LOG_ERROR_IF(condition, ...)

#endif

namespace microtrace {

inline int uv_fd(const uv_stream_t* stream) { return stream->io_watcher.fd; }
inline int uv_fd(const uv_tcp_t* tcp) {
    return uv_fd(reinterpret_cast<const uv_stream_t*>(tcp));
}

inline char* string_arr(std::string& str) { return &str[0]; }

extern std::shared_ptr<spdlog::logger> console_log;

unsigned short get_port(const struct sockaddr* sa);
}
