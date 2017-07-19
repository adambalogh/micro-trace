#pragma once

#include <memory>
#include <string>

#include <boost/uuid/uuid.hpp>

#include "request_log.pb.h"

namespace proto {
bool operator==(const proto::Uuid& a, const proto::Uuid& b);
bool operator!=(const proto::Uuid& a, const proto::Uuid& b);
}

namespace microtrace {

extern const char* CONTEXT_PREFIX;
extern const ssize_t CONTEXT_PREFIX_SIZE;

inline ssize_t ContextProtoSize() { return 3 * sizeof(uint64_t); }

/*
 * This module is responsible for keeping track of the trace associated with the
 * every thread of execution. The trace may change throughout the lifeteime of
 * each thread. By default, the trace is undefined.
 */

class Context {
   public:
    /*
     * Generates a random context.
     */
    Context();

    /*
     * Returns a context filled with the values of ctx.
     */
    Context(proto::Context ctx);

    static bool SameTrace(const Context& a, const Context& b) {
        return a.trace() == b.trace();
    }

    const proto::Uuid& trace() const { return context_.trace_id(); }
    const proto::Uuid& span() const { return context_.span_id(); }
    const proto::Uuid& parent_span() const { return context_.parent_span(); }

    /*
     * Generates and assigns a new span to this context, and saves the current
     * span as the parent span.
     */
    void NewSpan();

    std::string Serialize() const;

   private:
    proto::Context context_;
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
