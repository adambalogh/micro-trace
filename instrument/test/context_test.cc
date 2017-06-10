#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>
#include <thread>

#include "context.h"

using namespace microtrace;

TEST(Context, Unique) {
    auto first = Context{};
    auto second = Context{};

    EXPECT_EQ(first, first);
    EXPECT_EQ(second, second);
    EXPECT_NE(first, second);
}

TEST(Context, DefaultConstructor) {
    Context ctx;
    EXPECT_EQ(ctx.trace(), ctx.span());
    EXPECT_EQ(ctx.span(), ctx.parent_span());
}

TEST(Context, NewSpan) {
    Context ctx;
    const std::string orig_trace = ctx.trace();
    const std::string orig_span = ctx.span();

    ctx.NewSpan();
    EXPECT_EQ(orig_trace, ctx.trace());
    EXPECT_NE(orig_span, ctx.span());
}

TEST(Context, Equality) {
    Context first;
    Context second;

    EXPECT_NE(first, second);
    EXPECT_EQ(first, first);

    second.set_span(first.span());
    second.set_parent_span(second.parent_span());
    EXPECT_NE(second, first);
}

TEST(Context, CurrentTrace) {
    std::thread t1{[]() {
        EXPECT_TRUE(is_context_undefined());

        Context context{};
        set_current_context(context);

        EXPECT_FALSE(is_context_undefined());
        EXPECT_EQ(context, get_current_context());
    }};
    t1.join();
}

TEST(Context, UniquePerThread) {
    std::mutex mu;
    std::condition_variable cv;

    bool first_context_set = false;
    Context second;

    std::thread t1{[&]() {
        EXPECT_TRUE(is_context_undefined());

        {
            std::unique_lock<std::mutex> l(mu);
            cv.wait(l, [&first_context_set]() { return first_context_set; });
        }

        EXPECT_TRUE(is_context_undefined());
        second = Context{};
        set_current_context(second);
    }};

    std::thread t2{[&]() {
        EXPECT_TRUE(is_context_undefined());

        Context first{};
        set_current_context(first);

        {
            std::unique_lock<std::mutex> l(mu);
            first_context_set = true;
        }
        cv.notify_all();
        t1.join();

        EXPECT_NE(get_current_context(), second);
        EXPECT_EQ(first, get_current_context());
    }};

    t2.join();
}

TEST(Context, ErrorIfUndefined) {
    std::thread t1{[]() {
        ::testing::FLAGS_gtest_death_test_style = "threadsafe";
        EXPECT_DEATH(get_current_context(), "");
    }};
    t1.join();
}
