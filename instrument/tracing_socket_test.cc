#include "gtest/gtest.h"

#include "tracing_socket.h"

#include <netinet/in.h>
#include <sys/socket.h>

const int FD = 9943;
const trace_id_t TRACE = 43242;

TEST(Test, Init) {
    TracingSocket socket{FD, TRACE, SocketRole::CLIENT};
    EXPECT_EQ(FD, socket.fd());
}
