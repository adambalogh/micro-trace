#include "context.h"

#include <assert.h>
#include <bitset>
#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "common.h"

namespace proto {
bool operator==(const proto::Uuid& a, const proto::Uuid& b) {
    return a.low() == b.low() && a.high() == b.high();
}

bool operator!=(const proto::Uuid& a, const proto::Uuid& b) {
    return !operator==(a, b);
}
}

namespace microtrace {

// Should be able to differentiate between normal and
// context-prefixed messages
const char* CONTEXT_PREFIX = "CTX432z$";
const ssize_t CONTEXT_PREFIX_SIZE = 8;

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

static proto::Uuid NewUuid() {
    uint64_t low = 0;
    uint64_t high = 0;

    const auto uuid = new_boost_uuid();
    const int byte_size = 8;  // in bits

    static_assert(1 == sizeof(boost::uuids::uuid::value_type),
                  "Boost Uuid byte should be 8 bits");
    static_assert(16 == uuid.size(), "Boost Uuid should be 16 bytes");

    for (int i = 0; i < 8; ++i) {
        high = (high << byte_size);
        high |= *(uuid.begin() + i);
    }
    for (int i = 0; i < 8; ++i) {
        low = (low << byte_size);
        low |= *(uuid.begin() + 8 + i);
    }

    proto::Uuid ret;
    ret.set_low(low);
    ret.set_high(high);
    return ret;
}

Context::Context() {
    *context_.mutable_trace_id() = NewUuid();
    *context_.mutable_span_id() = context_.trace_id();
    *context_.mutable_parent_span() = context_.trace_id();
}

void Context::NewSpan() {
    *context_.mutable_parent_span() = context_.span_id();
    *context_.mutable_span_id() = NewUuid();
}

std::string Context::Serialize() const {
    std::string data;
    context_.SerializeToString(&data);
    return data;
}

bool operator==(const Context& a, const Context& b) {
    return a.trace() == b.trace() && a.span() == b.span();
}

bool operator!=(const Context& a, const Context& b) {
    return !operator==(a, b);
}
}
