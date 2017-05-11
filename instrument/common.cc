#include "common.h"

static thread_local trace_id_t current_trace = UNDEFINED_TRACE;

trace_id_t get_current_trace() { return current_trace; }

void set_current_trace(const trace_id_t trace) {
    if (valid_trace(trace)) {
        current_trace = trace;
    }
}
