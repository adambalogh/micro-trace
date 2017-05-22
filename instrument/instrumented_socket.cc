#include "instrumented_socket.h"

#include <assert.h>

#include "orig_functions.h"
#include "request_log.pb.h"

namespace microtrace {

static const int SINGLE_IOVEC = 1;

InstrumentedSocket::InstrumentedSocket(
    const int fd, std::unique_ptr<SocketCallback> event_handler,
    const OriginalFunctions &orig)
    : fd_(fd), callback_(std::move(event_handler)), orig_(orig) {
    assert(fd_ == callback_->fd());
}

void InstrumentedSocket::Accept() { callback_->AfterAccept(); }

// TODO handle special case when len == 0
ssize_t InstrumentedSocket::RecvFrom(void *buf, size_t len, int flags,
                                     struct sockaddr *src_addr,
                                     socklen_t *addrlen) {
    callback_->BeforeRead();
    ssize_t ret = orig_.recvfrom(fd(), buf, len, flags, src_addr, addrlen);
    callback_->AfterRead(buf, ret);
    return ret;
}

ssize_t InstrumentedSocket::Recv(void *buf, size_t len, int flags) {
    callback_->BeforeRead();
    ssize_t ret = orig_.recv(fd(), buf, len, flags);
    callback_->AfterRead(buf, ret);
    return ret;
}

ssize_t InstrumentedSocket::Read(void *buf, size_t count) {
    callback_->BeforeRead();
    ssize_t ret = orig_.read(fd(), buf, count);
    callback_->AfterRead(buf, ret);
    return ret;
}

ssize_t InstrumentedSocket::Send(const void *buf, size_t len, int flags) {
    callback_->BeforeWrite();
    ssize_t ret = orig_.send(fd(), buf, len, flags);
    callback_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    return ret;
}

ssize_t InstrumentedSocket::Write(const void *buf, size_t count) {
    callback_->BeforeWrite();
    ssize_t ret = orig_.write(fd(), buf, count);
    callback_->AfterWrite(set_iovec(buf, count), SINGLE_IOVEC, ret);
    return ret;
}

ssize_t InstrumentedSocket::Writev(const struct iovec *iov, int iovcnt) {
    callback_->BeforeWrite();
    ssize_t ret = orig_.writev(fd(), iov, iovcnt);
    callback_->AfterWrite(iov, iovcnt, ret);
    return ret;
}

ssize_t InstrumentedSocket::SendTo(const void *buf, size_t len, int flags,
                                   const struct sockaddr *dest_addr,
                                   socklen_t addrlen) {
    callback_->BeforeWrite();
    ssize_t ret = orig_.sendto(fd(), buf, len, flags, dest_addr, addrlen);
    callback_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    return ret;
}

int InstrumentedSocket::Close() {
    callback_->BeforeClose();
    int ret = orig_.close(fd());
    callback_->AfterClose(ret);
    return ret;
}
}
