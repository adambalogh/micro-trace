#include "context.h"

#include <assert.h>
#include <bitset>
#include <iostream>
#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "common.h"

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

static boost::uuids::uuid new_boost_uuid() {
    static thread_local boost::uuids::random_generator gen;
    return gen();
}

Uuid Uuid::Zero() { return Uuid{0, 0}; }

Uuid::Uuid() : high_(0), low_(0) {
    const auto uuid = new_boost_uuid();
    const int byte_size = 8;  // in bits

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

Uuid::Uuid(uint64_t high, uint64_t low) : high_(high), low_(low) {}

bool operator==(const Uuid& a, const Uuid& b) {
    return a.high() == b.high() && a.low() == b.low();
}

bool operator!=(const Uuid& a, const Uuid& b) { return !operator==(a, b); }

Context::Context() {}

Context::Context(ContextStorage ctx) : context_(std::move(ctx)) {}

void Context::NewSpan() {
    context_.parent_span = context_.span_id;
    context_.span_id = Uuid{};
}

bool operator==(const Context& a, const Context& b) {
    return a.trace() == b.trace() && a.span() == b.span();
}

bool operator!=(const Context& a, const Context& b) {
    return !operator==(a, b);
}
}
