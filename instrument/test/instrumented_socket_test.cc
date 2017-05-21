#include "gtest/gtest.h"

#include "instrumented_socket.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include "fakeit.hpp"

#include "orig_functions.h"
#include "test_util.h"

using namespace fakeit;

const int FD = 9943;
const trace_id_t TRACE{};

const size_t LEN = 11;
void* const BUF = new char[LEN];

const int FLAGS = 99;

struct sockaddr sa = {};
struct sockaddr* SOCKADDR = &sa;

socklen_t ADDRLEN = 10;
socklen_t* const ADDRLEN_PTR = &ADDRLEN;

const int SUCCESSFUL_CLOSE = 1;

const struct iovec iov = {};
const struct iovec* IOVEC = &iov;
const int IOVCNT = 1;

const int RET = 55;

const EmptyOriginalFunctions empty_orig;

TEST(InstrumentedSocketTest, Init) {
    InstrumentedSocket socket{
        FD, EmptySocketCallback::New(FD, TRACE, SocketRole::CLIENT),
        empty_orig};

    EXPECT_EQ(FD, socket.fd());
}

TEST(InstrumentedSocketTest, SocketApiCalls) {
    Mock<OriginalFunctions> mock;
    Method(mock, recv) = RET;
    Method(mock, read) = RET;
    Method(mock, recvfrom) = RET;
    Method(mock, write) = RET;
    Method(mock, writev) = RET;
    Method(mock, send) = RET;
    Method(mock, sendto) = RET;
    Method(mock, close) = SUCCESSFUL_CLOSE;

    OriginalFunctions& mock_orig = mock.get();
    InstrumentedSocket socket{
        FD, EmptySocketCallback::New(FD, TRACE, SocketRole::CLIENT), mock_orig};

    EXPECT_EQ(RET, socket.Read(BUF, LEN));
    Verify(Method(mock, read).Using(FD, BUF, LEN));

    EXPECT_EQ(RET, socket.Recv(BUF, LEN, FLAGS));
    Verify(Method(mock, recv).Using(FD, BUF, LEN, FLAGS));

    EXPECT_EQ(RET, socket.RecvFrom(BUF, LEN, FLAGS, SOCKADDR, ADDRLEN_PTR));
    Verify(Method(mock, recvfrom)
               .Using(FD, BUF, LEN, FLAGS, SOCKADDR, ADDRLEN_PTR));

    EXPECT_EQ(RET, socket.Write(BUF, LEN));
    Verify(Method(mock, write).Using(FD, BUF, LEN));

    EXPECT_EQ(RET, socket.Writev(IOVEC, IOVCNT));
    Verify(Method(mock, writev).Using(FD, IOVEC, IOVCNT));

    EXPECT_EQ(RET, socket.Send(BUF, LEN, FLAGS));
    Verify(Method(mock, send).Using(FD, BUF, LEN, FLAGS));

    EXPECT_EQ(RET, socket.SendTo(BUF, LEN, FLAGS, SOCKADDR, ADDRLEN));
    Verify(Method(mock, sendto).Using(FD, BUF, LEN, FLAGS, SOCKADDR, ADDRLEN));

    // Make sure to do this last
    EXPECT_EQ(SUCCESSFUL_CLOSE, socket.Close());
    Verify(Method(mock, close).Using(FD));

    // Verify that no other syscall was called
    VerifyNoOtherInvocations(mock);
}
