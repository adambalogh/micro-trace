#pragma once

#include <assert.h>
#include <string>

#define LOG(msg, ...)                                \
    pthread_t thread = pthread_self();               \
    printf("[t:%lu %d]: ", thread, __current_trace); \
    printf(msg, __VA_ARGS__);                        \
    printf("\n");

#ifdef DEBUG
#define DLOG(msg, ...) LOG(msg, __VA_ARGS__)
#else
#define DLOG(msg, ...)
#endif

#define UNDEFINED_TRACE -2

inline char* string_arr(std::string& str) { return &str[0]; }

typedef int32_t trace_id_t;

extern thread_local trace_id_t __current_trace;

inline int valid_trace(const trace_id_t trace) {
    return trace != UNDEFINED_TRACE && trace != -1;
}

inline trace_id_t get_current_trace() { return __current_trace; }

inline void set_current_trace(const trace_id_t trace) {
    if (valid_trace(trace)) {
        __current_trace = trace;
    }
}
