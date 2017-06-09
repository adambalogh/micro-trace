#include "socket_adapter.h"

#include <assert.h>

#include "orig_functions.h"
#include "request_log.pb.h"

namespace microtrace {

static const int SINGLE_IOVEC = 1;

SocketAdapter::SocketAdapter(const int fd,
                             std::unique_ptr<InstrumentedSocket> isock,
                             const OriginalFunctions &orig)
    : fd_(fd), isock_(std::move(isock)), orig_(orig) {
    assert(fd_ == isock_->fd());
}

// TODO handle special case when len == 0
ssize_t SocketAdapter::RecvFrom(void *buf, size_t len, int flags,
                                struct sockaddr *src_addr, socklen_t *addrlen) {
    IoFunction fun = [&, this]() {
        return orig_.recvfrom(fd(), buf, len, flags, src_addr, addrlen);
    };
    return isock_->Read(buf, len, fun);
}

ssize_t SocketAdapter::Recv(void *buf, size_t len, int flags) {
    IoFunction fun = [&, this]() { return orig_.recv(fd(), buf, len, flags); };
    return isock_->Read(buf, len, fun);
}

ssize_t SocketAdapter::Read(void *buf, size_t count) {
    IoFunction fun = [&, this]() { return orig_.read(fd(), buf, count); };
    return isock_->Read(buf, count, fun);
}

ssize_t SocketAdapter::Send(const void *buf, size_t len, int flags) {
    IoFunction fun = [&, this]() { return orig_.send(fd(), buf, len, flags); };
    return isock_->Write(set_iovec(buf, len), SINGLE_IOVEC, fun);
}

ssize_t SocketAdapter::Write(const void *buf, size_t count) {
    IoFunction fun = [&, this]() { return orig_.write(fd(), buf, count); };
    return isock_->Write(set_iovec(buf, count), SINGLE_IOVEC, fun);
}

ssize_t SocketAdapter::Writev(const struct iovec *iov, int iovcnt) {
    IoFunction fun = [&, this]() { return orig_.writev(fd(), iov, iovcnt); };
    return isock_->Write(iov, iovcnt, fun);
}

ssize_t SocketAdapter::SendTo(const void *buf, size_t len, int flags,
                              const struct sockaddr *dest_addr,
                              socklen_t addrlen) {
    IoFunction fun = [&, this]() {
        return orig_.sendto(this->fd(), buf, len, flags, dest_addr, addrlen);
    };
    return isock_->Write(set_iovec(buf, len), SINGLE_IOVEC, fun);
}

ssize_t SocketAdapter::SendMsg(const struct msghdr *msg, int flags) {
    IoFunction fun = [&, this]() {
        return orig_.sendmsg(this->fd(), msg, flags);
    };
    return isock_->Write(msg->msg_iov, msg->msg_iovlen, fun);
}

int SocketAdapter::Close() {
    CloseFunction fun = [this]() { return this->orig_.close(this->fd()); };
    return isock_->Close(fun);
}
}
