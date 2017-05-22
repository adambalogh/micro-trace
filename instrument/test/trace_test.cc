#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>
#include <thread>

#include "trace.h"

using namespace microtrace;

TEST(Trace, Unique) {
    auto first = new_trace();
    auto second = new_trace();

    EXPECT_EQ(first, first);
    EXPECT_EQ(second, second);
    EXPECT_NE(first, second);
}

TEST(Trace, CurrentTrace) {
    std::thread t1{[]() {
        EXPECT_TRUE(is_trace_undefined());

        trace_id_t trace = new_trace();
        set_current_trace(trace);

        EXPECT_FALSE(is_trace_undefined());
        EXPECT_EQ(trace, get_current_trace());
    }};
    t1.join();
}

TEST(Trace, UniquePerThread) {
    std::mutex mu;
    std::condition_variable cv;

    bool first_trace_set = false;
    trace_id_t second;

    std::thread t1{[&]() {
        EXPECT_TRUE(is_trace_undefined());

        {
            std::unique_lock<std::mutex> l(mu);
            cv.wait(l, [&first_trace_set]() { return first_trace_set; });
        }

        EXPECT_TRUE(is_trace_undefined());
        second = new_trace();
        set_current_trace(second);
    }};

    std::thread t2{[&]() {
        EXPECT_TRUE(is_trace_undefined());

        trace_id_t first = new_trace();
        set_current_trace(first);

        {
            std::unique_lock<std::mutex> l(mu);
            first_trace_set = true;
        }
        cv.notify_all();
        t1.join();

        EXPECT_NE(get_current_trace(), second);
        EXPECT_EQ(first, get_current_trace());
    }};

    t2.join();
}

TEST(Trace, ErrorIfUndefined) {
    std::thread t1{[]() {
        ::testing::FLAGS_gtest_death_test_style = "threadsafe";
        EXPECT_DEATH(get_current_trace(), "trace_undefined");
    }};
    t1.join();
}
