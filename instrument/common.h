#pragma once

#include <string>

#define LOG(msg, ...)                                      \
    pthread_t thread = pthread_self();                     \
    printf("[t:%lu %d]: ", thread % 10000, current_trace); \
    printf(msg, __VA_ARGS__);                              \
    printf("\n");

#ifdef DEBUG
#define DLOG(msg, ...) LOG(msg, __VA_ARGS__)
#else
#define DLOG(msg, ...)
#endif

#define UNDEFINED_TRACE -2

inline char* string_arr(std::string& str) { return &str[0]; }

typedef int32_t trace_id_t;

static __thread trace_id_t current_trace = UNDEFINED_TRACE;

static inline int valid_trace(const trace_id_t trace) {
    return trace != UNDEFINED_TRACE && trace != -1;
}

static inline void set_current_trace(const trace_id_t trace) {
    if (valid_trace(trace)) {
        current_trace = trace;
    }
}
