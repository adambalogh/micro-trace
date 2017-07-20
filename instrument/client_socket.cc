#include "client_socket.h"

#include <assert.h>

#include "orig_functions.h"
#include "request_log.pb.h"

namespace microtrace {

ClientSocket::ClientSocket(const int fd,
                           std::unique_ptr<ClientSocketHandler> handler,
                           const OriginalFunctions &orig)
    : InstrumentedSocket(fd, orig), handler_(std::move(handler)) {
    VERIFY(fd_ == handler_->fd(),
           "handler and underlying socket's fd is not the same");
    VERIFY(handler_->role_client(),
           "ClientSocket given a non-client socket handler");
}

void ClientSocket::Async() { handler_->Async(); }

ssize_t ClientSocket::RecvFrom(void *buf, size_t len, int flags,
                               struct sockaddr *src_addr, socklen_t *addrlen) {
    handler_->BeforeRead(buf, len);
    auto ret = orig_.recvfrom(fd(), buf, len, flags, src_addr, addrlen);
    handler_->AfterRead(buf, len, ret);
    return ret;
}

ssize_t ClientSocket::Recv(void *buf, size_t len, int flags) {
    handler_->BeforeRead(buf, len);
    auto ret = orig_.recv(fd(), buf, len, flags);
    handler_->AfterRead(buf, len, ret);
    return ret;
}

ssize_t ClientSocket::Read(void *buf, size_t count) {
    handler_->BeforeRead(buf, count);
    auto ret = orig_.read(fd(), buf, count);
    handler_->AfterRead(buf, count, ret);
    return ret;
}

ssize_t ClientSocket::Send(const void *buf, size_t len, int flags) {
    handler_->BeforeWrite(set_iovec(buf, len), SINGLE_IOVEC);
    auto ret = orig_.send(fd(), buf, len, flags);
    handler_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    return ret;
}

ssize_t ClientSocket::Write(const void *buf, size_t count) {
    handler_->BeforeWrite(set_iovec(buf, count), SINGLE_IOVEC);
    auto ret = orig_.write(fd(), buf, count);
    handler_->AfterWrite(set_iovec(buf, count), SINGLE_IOVEC, ret);
    return ret;
}

ssize_t ClientSocket::Writev(const struct iovec *iov, int iovcnt) {
    handler_->BeforeWrite(iov, iovcnt);
    auto ret = orig_.writev(fd(), iov, iovcnt);
    handler_->AfterWrite(iov, iovcnt, ret);
    return ret;
}

ssize_t ClientSocket::SendTo(const void *buf, size_t len, int flags,
                             const struct sockaddr *dest_addr,
                             socklen_t addrlen) {
    handler_->BeforeWrite(set_iovec(buf, len), SINGLE_IOVEC);
    auto ret = orig_.sendto(this->fd(), buf, len, flags, dest_addr, addrlen);
    handler_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    return ret;
}

ssize_t ClientSocket::SendMsg(const struct msghdr *msg, int flags) {
    handler_->BeforeWrite(msg->msg_iov, msg->msg_iovlen);
    auto ret = orig_.sendmsg(this->fd(), msg, flags);
    handler_->AfterWrite(msg->msg_iov, msg->msg_iovlen, ret);
    return ret;
}

int ClientSocket::Close() {
    handler_->BeforeClose();
    auto ret = orig_.close(this->fd());
    printf("before ClientSocket::AfterClose\n");
    handler_->AfterClose(ret);
    printf("after ClientSocket::AfterClose\n");
    return ret;
}
}
