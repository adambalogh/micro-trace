#include "instrumented_socket.h"

#include <assert.h>

#include "orig_functions.h"
#include "request_log.pb.h"

static const int SINGLE_IOVEC = 1;

InstrumentedSocket::InstrumentedSocket(
    const int fd, std::unique_ptr<SocketEventHandler> event_handler,
    const OriginalFunctions &orig)
    : fd_(fd), event_handler_(std::move(event_handler)), orig_(orig) {
    assert(fd_ == event_handler_->fd());
}

void InstrumentedSocket::Accept() { event_handler_->AfterAccept(); }

// TODO handle special case when len == 0
ssize_t InstrumentedSocket::RecvFrom(void *buf, size_t len, int flags,
                                     struct sockaddr *src_addr,
                                     socklen_t *addrlen) {
    event_handler_->BeforeRead();
    ssize_t ret = orig_.recvfrom(fd(), buf, len, flags, src_addr, addrlen);
    if (ret != -1) {
        event_handler_->AfterRead(buf, ret);
    }
    return ret;
}

ssize_t InstrumentedSocket::Recv(void *buf, size_t len, int flags) {
    event_handler_->BeforeRead();
    ssize_t ret = orig_.recv(fd(), buf, len, flags);
    if (ret != -1) {
        event_handler_->AfterRead(buf, ret);
    }
    return ret;
}

ssize_t InstrumentedSocket::Read(void *buf, size_t count) {
    event_handler_->BeforeRead();
    ssize_t ret = orig_.read(fd(), buf, count);
    if (ret != -1) {
        event_handler_->AfterRead(buf, ret);
    }
    return ret;
}

ssize_t InstrumentedSocket::Send(const void *buf, size_t len, int flags) {
    event_handler_->BeforeWrite();
    ssize_t ret = orig_.send(fd(), buf, len, flags);
    if (ret != -1) {
        event_handler_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    }
    return ret;
}

ssize_t InstrumentedSocket::Write(const void *buf, size_t count) {
    event_handler_->BeforeWrite();
    ssize_t ret = orig_.write(fd(), buf, count);
    if (ret != -1) {
        event_handler_->AfterWrite(set_iovec(buf, count), SINGLE_IOVEC, ret);
    }
    return ret;
}

ssize_t InstrumentedSocket::Writev(const struct iovec *iov, int iovcnt) {
    event_handler_->BeforeWrite();
    ssize_t ret = orig_.writev(fd(), iov, iovcnt);
    if (ret != -1) {
        event_handler_->AfterWrite(iov, iovcnt, ret);
    }
    return ret;
}

ssize_t InstrumentedSocket::SendTo(const void *buf, size_t len, int flags,
                                   const struct sockaddr *dest_addr,
                                   socklen_t addrlen) {
    event_handler_->BeforeWrite();
    ssize_t ret = orig_.sendto(fd(), buf, len, flags, dest_addr, addrlen);
    if (ret != -1) {
        event_handler_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    }
    return ret;
}

int InstrumentedSocket::Close() {
    event_handler_->BeforeClose();
    int ret = orig_.close(fd());
    event_handler_->AfterClose(ret);
    return ret;
}
