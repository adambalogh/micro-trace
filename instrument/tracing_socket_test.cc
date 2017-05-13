#include "gtest/gtest.h"

#include "tracing_socket.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>

#include "orig_functions.h"
#include "test_util.h"

#define LEN 10
#define FLAGS 4543
#define RET 8885
#define BUF nullptr
#define SOCKADDR nullptr
#define ADDRLEN 19
#define ADDRLEN_PTR nullptr
#define IOVEC nullptr
#define IOVCNT 0
#define SUCCESSFUL_CLOSE 0

const int FD = 9943;
const trace_id_t TRACE = 43242;

OriginalFunctions mock_orig = []() {
    OriginalFunctions orig;
    orig.orig_recvfrom = [](int fd, void* buf, size_t len, int flags,
                            struct sockaddr* src_addr,
                            socklen_t* addrlen) -> ssize_t {
        EXPECT_EQ(FD, fd);
        EXPECT_EQ(BUF, buf);
        EXPECT_EQ(LEN, len);
        EXPECT_EQ(FLAGS, flags);
        EXPECT_EQ(SOCKADDR, src_addr);
        EXPECT_EQ(ADDRLEN_PTR, addrlen);
        return RET;
    };
    orig.orig_recv = [](int fd, void* buf, size_t len, int flags) -> ssize_t {
        EXPECT_EQ(FD, fd);
        EXPECT_EQ(BUF, buf);
        EXPECT_EQ(LEN, len);
        EXPECT_EQ(FLAGS, flags);
        return RET;
    };
    orig.orig_read = [](int fd, void* buf, size_t len) -> ssize_t {
        EXPECT_EQ(FD, fd);
        EXPECT_EQ(BUF, buf);
        EXPECT_EQ(LEN, len);
        return RET;
    };
    orig.orig_write = [](int fd, const void* buf, size_t len) -> ssize_t {
        EXPECT_EQ(FD, fd);
        EXPECT_EQ(BUF, buf);
        EXPECT_EQ(LEN, len);
        return RET;
    };
    orig.orig_writev = [](int fd, const struct iovec* iov,
                          int iovcnt) -> ssize_t {
        EXPECT_EQ(FD, fd);
        EXPECT_EQ(IOVEC, iov);
        EXPECT_EQ(IOVCNT, iovcnt);
        return RET;
    };
    orig.orig_send = [](int fd, const void* buf, size_t len,
                        int flags) -> ssize_t {
        EXPECT_EQ(FD, fd);
        EXPECT_EQ(BUF, buf);
        EXPECT_EQ(LEN, len);
        EXPECT_EQ(FLAGS, flags);
        return RET;
    };
    orig.orig_sendto = [](int fd, const void* buf, size_t len, int flags,
                          const struct sockaddr* dest_addr,
                          socklen_t addrlen) -> ssize_t {
        EXPECT_EQ(FD, fd);
        EXPECT_EQ(BUF, buf);
        EXPECT_EQ(LEN, len);
        EXPECT_EQ(FLAGS, flags);
        EXPECT_EQ(SOCKADDR, dest_addr);
        EXPECT_EQ(ADDRLEN, addrlen);
        return RET;
    };
    orig.orig_close = [](int fd) -> int {
        EXPECT_EQ(FD, fd);
        return SUCCESSFUL_CLOSE;
    };

    return orig;
}();

TEST(TracingSocket, Init) {
    TracingSocket socket{
        FD, MakeEmptySocketEventHandler(FD, TRACE, SocketRole::CLIENT),
        empty_orig};

    EXPECT_EQ(FD, socket.fd());
}

TEST(TracingSocketTest, SocketApiCalls) {
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
