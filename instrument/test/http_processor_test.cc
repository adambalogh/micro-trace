#include <gtest/gtest.h>

#include "http_processor.h"

using namespace microtrace;

TEST(HttpProcessor, Match) {
    bool called = false;
    HttpProcessor p{[&called](std::string url) {
        called = true;
        EXPECT_EQ("http://www.google.com", url);
    }};

    const char* msg = "GET http://www.google.com rega";
    const int MSG_LEN = 30;

    EXPECT_TRUE(p.Process(msg, 1));
    msg += 1;
    EXPECT_TRUE(p.Process(msg, 1));
    msg += 1;
    EXPECT_FALSE(p.Process(msg, 24));

    EXPECT_TRUE(called);
}

TEST(HttpProcessor, InvalidMethod) {
    bool called = false;
    HttpProcessor p{[&called](std::string url) { called = true; }};

    const char* msg = "YO http://www.google.com rega";
    const int MSG_LEN = 30;

    EXPECT_FALSE(p.Process(msg, 1));
    EXPECT_FALSE(called);
}

TEST(HttpProcessor, NoSpaceAfterURL) {
    bool called = false;
    HttpProcessor p{[&called](std::string url) { called = true; }};

    const char* msg = "GET http://www.google.com";

    EXPECT_TRUE(p.Process(msg, 1));
    msg += 1;
    EXPECT_TRUE(p.Process(msg, 1));
    msg += 1;
    EXPECT_TRUE(p.Process(msg, 22));

    EXPECT_FALSE(called);
}
