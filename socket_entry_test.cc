#include "gtest/gtest.h"

#include "socket_entry.h"

#include <netinet/in.h>
#include <sys/socket.h>

const int FD = 9943;
const trace_id_t TRACE = 43242;

TEST(Connid, Init) {
    Connid cid;

    EXPECT_EQ(INET6_ADDRSTRLEN, cid.local_ip.size());
    EXPECT_EQ(INET6_ADDRSTRLEN, cid.peer_ip.size());
}

TEST(SocketEntryTest, Init) {
    SocketEntry socket{FD, TRACE, SocketEntry::SOCKET_OPENED};

    EXPECT_EQ(FD, socket.fd());
    EXPECT_EQ(TRACE, socket.trace());
    EXPECT_TRUE(socket.type_opened());
    EXPECT_FALSE(socket.has_connid());
}

TEST(SocketEntryTest, SetConnid) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    SocketEntry socket{fd, TRACE, SocketEntry::SOCKET_OPENED};
    int ret = socket.SetConnid();
}
