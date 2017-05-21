#include <gtest/gtest.h>

#include "trace.h"

TEST(Trace, Unique) {
    auto first = new_trace();
    auto second = new_trace();

    EXPECT_EQ(first, first);
    EXPECT_EQ(second, second);
    EXPECT_NE(first, second);
}

TEST(Trace, CurrentTrace) {
    EXPECT_TRUE(is_trace_undefined());

    trace_id_t trace = new_trace();
    set_current_trace(trace);

    EXPECT_FALSE(is_trace_undefined());
    EXPECT_EQ(trace, get_current_trace());
}
