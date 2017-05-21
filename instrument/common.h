#pragma once

#include <string>

#define LOG(msg, ...)                                    \
    pthread_t thread = pthread_self();                   \
    printf("[t:%lu %d]: ", thread, get_current_trace()); \
    printf(msg, __VA_ARGS__);                            \
    printf("\n");

#ifdef DEBUG
#define DLOG(msg, ...) LOG(msg, __VA_ARGS__)
#else
#define DLOG(msg, ...)
#endif

inline char* string_arr(std::string& str) { return &str[0]; }
