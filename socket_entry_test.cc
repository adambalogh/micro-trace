#include "gtest/gtest.h"

#include "socket_entry.h"

const int FD = 9943;
const trace_id_t TRACE = 43242;

TEST(SocketEntryTest, Init) {
    SocketEntry socket{FD, TRACE, SocketEntry::SOCKET_OPENED};

    EXPECT_EQ(FD, socket.fd());
    EXPECT_EQ(TRACE, socket.trace());
    EXPECT_TRUE(socket.type_opened());
    EXPECT_FALSE(socket.has_connid());
}
