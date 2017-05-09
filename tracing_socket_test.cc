#include "gtest/gtest.h"

#include "tracing_socket.h"

#include <netinet/in.h>
#include <sys/socket.h>

const int FD = 9943;
const trace_id_t TRACE = 43242;

TEST(Connid, Init) {
    Connid cid;

    EXPECT_EQ(INET6_ADDRSTRLEN, cid.local_ip.size());
    EXPECT_EQ(INET6_ADDRSTRLEN, cid.peer_ip.size());
}

TEST(TracingSocketTest, Init) {
    TracingSocket socket{FD, TRACE, SocketRole::CLIENT};

    EXPECT_EQ(FD, socket.fd());
    EXPECT_EQ(TRACE, socket.trace());
    EXPECT_TRUE(socket.role_client());
    EXPECT_FALSE(socket.has_connid());
}

TEST(TracingSocketTest, SetConnid) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    TracingSocket socket{fd, TRACE, SocketRole::CLIENT};
    int ret = socket.SetConnid();
}
