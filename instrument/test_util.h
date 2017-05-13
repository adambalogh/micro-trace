#pragma once

#include "orig_functions.h"
#include "socket_event_handler.h"

OriginalFunctions empty_orig = []() {
    OriginalFunctions orig;
    orig.orig_recvfrom = [](int fd, void* buf, size_t len, int flags,
                            struct sockaddr* src_addr,
                            socklen_t* addrlen) -> ssize_t { return -1; };
    orig.orig_recv = [](int fd, void* buf, size_t len, int flags) -> ssize_t {
        return -1;
    };
    orig.orig_read = [](int fd, void* buf, size_t len) -> ssize_t {
        return -1;
    };
    orig.orig_write = [](int fd, const void* buf, size_t len) -> ssize_t {
        return -1;
    };
    orig.orig_writev = [](int fd, const struct iovec* iov,
                          int iovcnt) -> ssize_t { return -1; };
    orig.orig_send = [](int fd, const void* buf, size_t len,
                        int flags) -> ssize_t { return -1; };
    orig.orig_sendto = [](int fd, const void* buf, size_t len, int flags,
                          const struct sockaddr* dest_Addr,
                          socklen_t addrlen) -> ssize_t { return -1; };
    orig.orig_close = [](int fd) -> int { return -1; };
    return orig;
}();

class EmptySocketEventHandler : public SocketEventHandler {
   public:
    EmptySocketEventHandler(int fd, const trace_id_t trace,
                            const SocketRole role)
        : SocketEventHandler(fd, trace, role) {}

    virtual void AfterAccept() override {}

    virtual void BeforeRead() override {}
    virtual void AfterRead(const void* buf, size_t ret) override {}
    virtual void BeforeWrite() override {}
    virtual void AfterWrite(const struct iovec* iov, int iovcnt,
                            ssize_t ret) override {}
    virtual void BeforeClose() override {}
    virtual void AfterClose(int ret) override {}
};

std::unique_ptr<SocketEventHandler> MakeEmptySocketEventHandler(
    int fd, const trace_id_t trace, const SocketRole role) {
    return std::make_unique<EmptySocketEventHandler>(fd, trace, role);
}
