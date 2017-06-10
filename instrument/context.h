#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

namespace microtrace {

/*
 * This module is responsible for keeping track of the trace associated with the
 * every thread of execution. The trace may change throughout the lifeteime of
 * each thread. By default, the trace is undefined.
 */

typedef boost::uuids::uuid uuid_t;

class Context {
   public:
    Context();

    std::string to_string() const;

    const std::string& trace() const { return trace_; }

    void set_span(const std::string& span) { span_ = span; }
    const std::string& span() const { return span_; }

    void set_parent_span(const std::string& parent_span) {
        parent_span_ = parent_span;
    }
    const std::string& parent_span() const { return parent_span_; }

    /*
     * Generates and assigns a new span to this context, and saves the current
     * span as the parent span.
     */
    void NewSpan();

   private:
    /*
     * A trace uniquely identifies a user request.
     */
    std::string trace_;

    /*
     * A span, along with a trace, identifies an action that can be attributed
     * to that trace.
     */
    std::string span_;

    /*
     * The span that *probably* caused the current span. The parent span must
     * belong to the same trace as the current one.
     */
    std::string parent_span_;
};

bool operator==(const Context& a, const Context& b);
bool operator!=(const Context& a, const Context& b);

/*
 * Returns the calling thread's currently associated context.
 * Must only be called if is_trace_undefined() is false.
 */
const Context& get_current_context();

/*
 * Sets the current context.
 */
void set_current_context(const Context context);

/*
 * Returns true if the current context is not defined.
 */
bool is_context_undefined();
}
