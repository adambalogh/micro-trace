#include "gtest/gtest.h"

#include "socket_callback.h"

const int FD = 11;
const trace_id_t TRACE{};
const SocketRole ROLE = SocketRole::SERVER;

TEST(SocketCallbackTest, Init) {
    SocketCallback cb{FD, TRACE, ROLE};
    EXPECT_EQ(FD, cb.fd());
}

TEST(SocketCallbackTest, ReadFirstInClientRole) {
    SocketCallback cb{FD, TRACE, SocketRole::CLIENT};
    cb.BeforeRead();
}

TEST(SocketCallbackTest, SetConnectionFailsWithInvalidFd) {
    SocketCallback eb{FD, TRACE, ROLE};
}
