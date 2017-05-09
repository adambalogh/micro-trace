#pragma once

#include <string>

// These macros act as a form of documentation
//  - OWNS means that the current struct owns ptr, so it must
//    free it when it's being destroyed.
//  - BORROWS means that the struct just borrowed the ptr, it
//    must not free it.
#define OWNS(ptr) ptr
#define BORROWS(ptr) ptr

// IMPORTANT evaluating expressions a and b multiple times shouldn't
// have any side-effect
#define MIN(a, b) ((a) < (b)) ? (a) : (b)

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