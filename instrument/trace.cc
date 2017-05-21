#include "trace.h"

#include <boost/uuid/uuid_io.hpp>

#define UNDEFINED_TRACE -2

static thread_local trace_id_t current_trace;
static thread_local bool trace_undefined = true;

trace_id_t get_current_trace() {
    assert(!trace_undefined);
    return current_trace;
}

void set_current_trace(const trace_id_t trace) {
    trace_undefined = false;
    current_trace = trace;
}

bool is_trace_undefined() { return trace_undefined; }

std::string trace_to_string(const trace_id_t trace) {
    return boost::uuids::to_string(trace);
}
