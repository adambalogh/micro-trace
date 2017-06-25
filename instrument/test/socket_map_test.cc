#include <gtest/gtest.h>

#include "socket_map.h"

#include "test_util.h"

using namespace microtrace;

const EmptyOriginalFunctions empty_orig;

TEST(SocketMapTest, InitiallyEmpty) {
    SocketMap map;
    for (int i = 0; i < SocketMap::DEFAULT_SIZE; ++i) {
        EXPECT_EQ(nullptr, map.Get(i));
    }
}

TEST(SocketMapTest, SetGetDelete) {
    SocketMap map;

    auto socket = std::make_unique<InstrumentedSocket>(
        0, DumbSocketHandler::New(0, SocketRole::CLIENT), empty_orig);

    map.Set(0, std::move(socket));
    EXPECT_NE(nullptr, map.Get(0));

    for (int i = 1; i < SocketMap::DEFAULT_SIZE; ++i) {
        EXPECT_EQ(nullptr, map.Get(i));
    }

    map.Delete(0);
    EXPECT_EQ(nullptr, map.Get(0));
}

TEST(SocketMapTest, Resize) {
    SocketMap map;

    // Add first socket in range
    auto first_socket = std::make_unique<InstrumentedSocket>(
        0, DumbSocketHandler::New(0, SocketRole::CLIENT), empty_orig);
    map.Set(0, std::move(first_socket));
    auto* first_ptr = map.Get(0);

    // Add second socket out of range
    const int out_of_range = SocketMap::DEFAULT_SIZE * 10;
    auto second_socket = std::make_unique<InstrumentedSocket>(
        out_of_range, DumbSocketHandler::New(out_of_range, SocketRole::CLIENT),
        empty_orig);
    map.Set(out_of_range, std::move(second_socket));
    auto* second_ptr = map.Get(out_of_range);
    EXPECT_NE(nullptr, second_ptr);
    EXPECT_NE(first_ptr, second_ptr);

    EXPECT_EQ(first_ptr, map.Get(0));
    for (int i = 1; i < out_of_range; ++i) {
        EXPECT_EQ(nullptr, map.Get(i));
    }
}

// We make sure that if an out of range fd is used, it doesn't throw any errors
TEST(SocketMapTest, OutOfRange) {
    SocketMap map;

    EXPECT_EQ(nullptr, map.Get(SocketMap::DEFAULT_SIZE * 100));
    map.Delete(SocketMap::DEFAULT_SIZE * 20);
}
