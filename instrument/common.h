#pragma once

#include <assert.h>
#include <string>

#include <boost/uuid/uuid.hpp>

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

typedef boost::uuids::uuid trace_id_t;

/*
 * Returns the calling thread's currently associated trace.
 */
trace_id_t get_current_trace();

/*
 * Sets the trace for the calling thread.
 */
void set_current_trace(const trace_id_t trace);

/*
 * Returns true if the current trace is not defined.
 */
bool is_trace_undefined();

std::string trace_to_string(const trace_id_t& trace);
