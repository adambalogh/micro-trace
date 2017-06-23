#pragma once

#include <memory>
#include <string>

#include <boost/uuid/uuid.hpp>

namespace microtrace {

/*
 * This module is responsible for keeping track of the trace associated with the
 * every thread of execution. The trace may change throughout the lifeteime of
 * each thread. By default, the trace is undefined.
 */

/*
 * An Uuid is derived from a boost::uuids::uuids object, its lower 64 bits
 * is stored in low_, and upper 64 bits in high_.
 */
class Uuid {
   public:
    Uuid();

    uint64_t high() const { return high_; }
    uint64_t low() const { return low_; }

   private:
    uint64_t high_;
    uint64_t low_;
};

bool operator==(const Uuid& a, const Uuid& b);
bool operator!=(const Uuid& a, const Uuid& b);

typedef Uuid uuid_t;

class Context {
   public:
    Context();

    static bool SameTrace(const Context& a, const Context& b) {
        return a.trace() == b.trace();
    }

    const uuid_t& trace() const { return trace_; }
    const uuid_t& span() const { return span_; }
    const uuid_t& parent_span() const { return parent_span_; }

    /*
     * Generates and assigns a new span to this context, and saves the current
     * span as the parent span.
     */
    void NewSpan();

   private:
    /*
     * A trace uniquely identifies a user request.
     */
    uuid_t trace_;

    /*
     * A span, along with a trace, identifies an action that can be attributed
     * to that trace.
     */
    uuid_t span_;

    /*
     * The span that *probably* caused the current span. The parent span must
     * belong to the same trace as the current one.
     */
    uuid_t parent_span_;
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
void set_current_context(const Context& context);

/*
 * Returns true if the current context is not defined.
 */
bool is_context_undefined();
}
