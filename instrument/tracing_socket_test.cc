#include "gtest/gtest.h"

#include "tracing_socket.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include "fakeit.hpp"

#include "orig_functions.h"
#include "test_util.h"

using namespace fakeit;

const int FD = 9943;
const int FLAGS = 99;
const size_t LEN = 11;
void* BUF = nullptr;
struct sockaddr* SOCKADDR = nullptr;
socklen_t ADDRLEN = 10;
socklen_t* ADDRLEN_PTR = nullptr;
const int SUCCESSFUL_CLOSE = 1;
const struct iovec* IOVEC = nullptr;
const int IOVCNT = 0;
const int RET = 1000;
const trace_id_t TRACE = 43242;

const EmptyOriginalFunctions empty_orig;

TEST(TracingSocket, Init) {
    TracingSocket socket{
        FD, MakeEmptySocketEventHandler(FD, TRACE, SocketRole::CLIENT),
        empty_orig};

    EXPECT_EQ(FD, socket.fd());
}

TEST(TracingSocketTest, SocketApiCalls) {
    Mock<OriginalFunctions> mock;
    Method(mock, socket) = RET;
    Method(mock, close) = RET;
    Method(mock, accept) = RET;
    Method(mock, accept4) = RET;
    Method(mock, recv) = RET;
    Method(mock, read) = RET;
    Method(mock, recvfrom) = RET;
    Method(mock, write) = RET;
    Method(mock, writev) = RET;
    Method(mock, send) = RET;
    Method(mock, sendto) = RET;
    Method(mock, uv_accept) = RET;
    Method(mock, uv_getaddrinfo) = RET;

    OriginalFunctions& mock_orig = mock.get();
    TracingSocket socket{
        FD, MakeEmptySocketEventHandler(FD, TRACE, SocketRole::CLIENT),
        mock_orig};

    EXPECT_EQ(RET, socket.Read(BUF, LEN));
    EXPECT_EQ(RET, socket.Recv(BUF, LEN, FLAGS));
    EXPECT_EQ(RET, socket.RecvFrom(BUF, LEN, FLAGS, SOCKADDR, ADDRLEN_PTR));
    EXPECT_EQ(RET, socket.Write(BUF, LEN));
    EXPECT_EQ(RET, socket.Writev(IOVEC, IOVCNT));
    EXPECT_EQ(RET, socket.Send(BUF, LEN, FLAGS));
    EXPECT_EQ(RET, socket.SendTo(BUF, LEN, FLAGS, SOCKADDR, ADDRLEN));

    // Make sure to do this last
    EXPECT_EQ(SUCCESSFUL_CLOSE, socket.Close());
}
