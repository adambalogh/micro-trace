#include "gtest/gtest.h"

#include "socket_callback.h"

const int FD = 11;
const trace_id_t TRACE{};
const SocketRole ROLE = SocketRole::SERVER;

TEST(SocketCallbackTest, Init) {
    SocketCallback ev{FD, TRACE, ROLE};
    EXPECT_EQ(FD, ev.fd());
}

TEST(SocketCallbackTest, SetConnectionFailsWithInvalidFd) {
    SocketCallback ev{FD, TRACE, ROLE};
    ev.AfterRead(nullptr, 0);
}
