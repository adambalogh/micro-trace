#pragma once

#include <assert.h>
#include <string>

#include <boost/uuid/uuid.hpp>

/*
 * This module is responsible for keeping track of the trace associated with the
 * current thread of execution. By default, the trace is undefined.
 */

typedef boost::uuids::uuid trace_id_t;

/*
 * Returns the calling thread's currently associated trace.
 * Must only be called if is_trace_undefined() is false.
 */
trace_id_t get_current_trace();

/*
 * Sets the current trace.
 */
void set_current_trace(const trace_id_t trace);

/*
 * Returns true if the current trace is not defined.
 */
bool is_trace_undefined();

std::string trace_to_string(const trace_id_t& trace);
