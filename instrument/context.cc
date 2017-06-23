#include "context.h"

#include <assert.h>
#include <bitset>
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

void set_current_context(const Context& context) {
    context_undefined = false;
    current_context = context;
}

bool is_context_undefined() { return context_undefined; }

static boost::uuids::uuid boost_uuid() {
    static thread_local boost::uuids::random_generator gen;
    return gen();
}

Uuid::Uuid() : high_(0), low_(0) {
    const auto uuid = boost_uuid();
    const int byte_size = 8;

    static_assert(1 == sizeof(boost::uuids::uuid::value_type),
                  "Boost Uuid byte should be 8 bits");
    static_assert(16 == uuid.size(), "Boost Uuid should be 16 bytes");

    for (int i = 0; i < 8; ++i) {
        high_ = (high_ << byte_size);
        high_ |= *(uuid.begin() + i);
    }
    for (int i = 0; i < 8; ++i) {
        low_ = (low_ << byte_size);
        low_ |= *(uuid.begin() + 8 + i);
    }
}

bool operator==(const Uuid& a, const Uuid& b) {
    return a.high() == b.high() && a.low() == b.low();
}

bool operator!=(const Uuid& a, const Uuid& b) { return !operator==(a, b); }

Context::Context() : trace_(Uuid{}), span_(trace_), parent_span_(trace_) {}

void Context::NewSpan() {
    parent_span_ = span_;
    span_ = Uuid{};
}

// std::string Context::to_string() const {
//    return "[trace_id: " + *trace_ + ", span: " + *span_ + ", parent_span:" +
//           *parent_span_ + "]";
//}

bool operator==(const Context& a, const Context& b) {
    return a.trace() == b.trace() && a.span() == b.span();
}

bool operator!=(const Context& a, const Context& b) {
    return !operator==(a, b);
}
}
