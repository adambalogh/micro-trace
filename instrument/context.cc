#include "context.h"

#include <assert.h>
#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "common.h"

#define UNDEFINED_TRACE -2

namespace microtrace {

static thread_local Context current_context;
static thread_local bool context_undefined = true;

const Context& get_current_context() {
    VERIFY(!context_undefined,
           "get_current_context called when context is undefined");
    return current_context;
}

void set_current_context(const Context context) {
    context_undefined = false;
    current_context = std::move(context);
}

bool is_context_undefined() { return context_undefined; }

// TODO random_generator is not thread-safe
uuid_t new_uuid() {
    static boost::uuids::random_generator gen;
    return gen();
}

Context::Context()
    : trace_(boost::uuids::to_string(new_uuid())),
      span_(trace_),
      parent_span_(trace_) {}

void Context::NewSpan() {
    parent_span_ = span_;
    span_ = boost::uuids::to_string(new_uuid());
}

std::string Context::to_string() const {
    return "[trace_id: " + trace_ + ", span: " + span_ + "]";
}

bool operator==(const Context& a, const Context& b) {
    return a.trace() == b.trace() && a.span() == b.span();
}

bool operator!=(const Context& a, const Context& b) {
    return !operator==(a, b);
}
}
