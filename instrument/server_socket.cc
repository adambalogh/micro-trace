#include "server_socket.h"

#include <assert.h>

#include "orig_functions.h"
#include "request_log.pb.h"

namespace microtrace {

ServerSocket::ServerSocket(const int fd, std::unique_ptr<SocketHandler> handler,
                           const OriginalFunctions &orig)
    : InstrumentedSocket(fd, std::move(handler), orig) {
    VERIFY(handler_->role_server(),
           "ServerSocket given a non-server socket handler");
}

void ServerSocket::Async() { handler_->Async(); }

ssize_t ServerSocket::RecvFrom(void *buf, size_t len, int flags,
                               struct sockaddr *src_addr, socklen_t *addrlen) {
    handler_->BeforeRead(buf, len);
    auto ret = orig_.recvfrom(fd(), buf, len, flags, src_addr, addrlen);
    handler_->AfterRead(buf, len, ret);
    return ret;
}

ssize_t ServerSocket::Recv(void *buf, size_t len, int flags) {
    handler_->BeforeRead(buf, len);
    auto ret = orig_.recv(fd(), buf, len, flags);
    handler_->AfterRead(buf, len, ret);
    return ret;
}

ssize_t ServerSocket::Read(void *buf, size_t count) {
    handler_->BeforeRead(buf, count);
    auto ret = orig_.read(fd(), buf, count);
    handler_->AfterRead(buf, count, ret);
    return ret;
}

ssize_t ServerSocket::Send(const void *buf, size_t len, int flags) {
    handler_->BeforeWrite(set_iovec(buf, len), SINGLE_IOVEC);
    auto ret = orig_.send(fd(), buf, len, flags);
    handler_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    return ret;
}

ssize_t ServerSocket::Write(const void *buf, size_t count) {
    handler_->BeforeWrite(set_iovec(buf, count), SINGLE_IOVEC);
    auto ret = orig_.write(fd(), buf, count);
    handler_->AfterWrite(set_iovec(buf, count), SINGLE_IOVEC, ret);
    return ret;
}

ssize_t ServerSocket::Writev(const struct iovec *iov, int iovcnt) {
    handler_->BeforeWrite(iov, iovcnt);
    auto ret = orig_.writev(fd(), iov, iovcnt);
    handler_->AfterWrite(iov, iovcnt, ret);
    return ret;
}

ssize_t ServerSocket::SendTo(const void *buf, size_t len, int flags,
                             const struct sockaddr *dest_addr,
                             socklen_t addrlen) {
    handler_->BeforeWrite(set_iovec(buf, len), SINGLE_IOVEC);
    auto ret = orig_.sendto(this->fd(), buf, len, flags, dest_addr, addrlen);
    handler_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    return ret;
}

ssize_t ServerSocket::SendMsg(const struct msghdr *msg, int flags) {
    handler_->BeforeWrite(msg->msg_iov, msg->msg_iovlen);
    auto ret = orig_.sendmsg(this->fd(), msg, flags);
    handler_->AfterWrite(msg->msg_iov, msg->msg_iovlen, ret);
    return ret;
}

int ServerSocket::Close() {
    handler_->BeforeClose();
    auto ret = orig_.close(this->fd());
    handler_->AfterClose(ret);
    return ret;
}
}
