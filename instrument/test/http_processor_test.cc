#include <gtest/gtest.h>

#include "http_processor.h"

using namespace microtrace;

TEST(HttpProcessor, Match) {
    HttpProcessor p;

    const char* msg = "GET http://www.google.com rega";
    const int MSG_LEN = 30;

    EXPECT_TRUE(p.Process(msg, 1));
    msg += 1;
    EXPECT_TRUE(p.Process(msg, 1));
    msg += 1;
    EXPECT_FALSE(p.Process(msg, 24));

    EXPECT_TRUE(p.has_url());
    EXPECT_EQ("http://www.google.com", p.url());
}

TEST(HttpProcessor, InvalidMethod) {
    HttpProcessor p;

    const char* msg = "YO http://www.google.com rega";
    const int MSG_LEN = 30;

    EXPECT_FALSE(p.Process(msg, 1));
    EXPECT_FALSE(p.has_url());
}

TEST(HttpProcessor, NoSpaceAfterURL) {
    HttpProcessor p;

    const char* msg = "GET http://www.google.com";

    EXPECT_TRUE(p.Process(msg, 1));
    msg += 1;
    EXPECT_TRUE(p.Process(msg, 1));
    msg += 1;
    EXPECT_TRUE(p.Process(msg, 22));

    EXPECT_FALSE(p.has_url());
}
