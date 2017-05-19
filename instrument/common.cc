#include "common.h"

#define UNDEFINED_TRACE -2

static thread_local trace_id_t current_trace = UNDEFINED_TRACE;

trace_id_t get_current_trace() { return current_trace; }

void set_current_trace(const trace_id_t trace) {
    if (valid_trace(trace)) {
        current_trace = trace;
    }
}

int valid_trace(const trace_id_t trace) {
    return trace != UNDEFINED_TRACE && trace != -1;
}

bool is_undefined_trace(const trace_id_t trace) {
    return trace == UNDEFINED_TRACE;
}
