#include "gtest/gtest.h"

#include <assert.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "common.h"

const int SERVER_PORT = 9999;

TEST(Trace, EmptyTraceByDefault) {
    EXPECT_EQ(UNDEFINED_TRACE, get_current_trace());
}

TEST(Trace, Socket) {
    int ret;

    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = SERVER_PORT;
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(server, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    assert(ret == 0);

    int client = socket(AF_INET, SOCK_STREAM, 0);
}
