#pragma once

#include <memory>
#include <string>

#include <boost/uuid/uuid.hpp>

#include "request_log.pb.h"

namespace microtrace {

/*
 * An Uuid is derived from a boost::uuids::uuids object, its lower 64 bits
 * is stored in low_, and upper 64 bits in high_.
 */
class Uuid {
   public:
    static Uuid Zero();

    Uuid();

    uint64_t high() const { return high_; }
    uint64_t low() const { return low_; }

    bool is_zero() const { return high_ == 0 && low_ == 0; }

    std::string to_string() const {
        return std::to_string(high_) + std::to_string(low_);
    }

   private:
    Uuid(uint64_t, uint64_t);

    uint64_t high_;
    uint64_t low_;
};

bool operator==(const Uuid& a, const Uuid& b);
bool operator!=(const Uuid& a, const Uuid& b);

typedef Uuid uuid_t;

/*
 * This module is responsible for keeping track of the trace associated with the
 * every thread of execution. The trace may change throughout the lifeteime of
 * each thread. By default, the trace is undefined.
 */

struct ContextStorage {
    static ContextStorage Zero() {
        return ContextStorage{uuid_t::Zero(), uuid_t::Zero(), uuid_t::Zero()};
    }

    ContextStorage()
        : trace_id(Uuid{}), span_id(Uuid{}), parent_span(trace_id) {}

    std::string to_string() const {
        return "trace_id: " + trace_id.to_string() + "\nspan_id: " +
               span_id.to_string() + "\nparent_span: " +
               parent_span.to_string();
    }

    uuid_t trace_id;
    uuid_t span_id;
    uuid_t parent_span;

   private:
    ContextStorage(uuid_t trace_id, uuid_t span_id, uuid_t parent_span)
        : trace_id(trace_id), span_id(span_id), parent_span(parent_span) {}
};

static_assert(sizeof(ContextStorage) == sizeof(uint64_t) * 2 * 3,
              "ContextStorage must be POD");

class Context {
   public:
    /*
     * Returns a context that has zero in all its values
     */
    static std::unique_ptr<Context> Zero() {
        return std::make_unique<Context>(ContextStorage::Zero());
    }

    /*
     * Generates a random context.
     */
    Context();

    /*
     * Returns a context filled with the values of ctx.
     */
    Context(ContextStorage ctx);

    static bool IsSameTrace(const Context& a, const Context& b) {
        return a.trace() == b.trace();
    }

    bool IsChildOf(const Context& parent) const {
        return trace() == parent.trace() && parent_span() == parent.span() &&
               span() != parent.span();
    }

    const uuid_t& trace() const { return context_.trace_id; }
    const uuid_t& span() const { return context_.span_id; }
    const uuid_t& parent_span() const { return context_.parent_span; }

    /*
     * Returns true if all the fields of this context are zero.
     */
    bool is_zero() const {
        return trace().is_zero() && span().is_zero() && parent_span().is_zero();
    }

    /*
     * Generates and assigns a new span to this context, and saves the current
     * span as the parent span.
     */
    void NewSpan();

    const ContextStorage& storage() const { return context_; }

   private:
    ContextStorage context_;
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
