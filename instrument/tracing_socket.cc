#include "tracing_socket.h"

#include "orig_functions.h"
#include "request_log.pb.h"

static const int SINGLE_IOVEC = 1;

TracingSocket::TracingSocket(const int fd, const trace_id_t trace,
                             const SocketRole role)
    : fd_(fd) {
    event_handler_.reset(new SocketEventHandler(*this, trace, role));
}

// TODO handle special case when len == 0
ssize_t TracingSocket::RecvFrom(void *buf, size_t len, int flags,
                                struct sockaddr *src_addr, socklen_t *addrlen) {
    event_handler_->BeforeRead();
    ssize_t ret =
        orig().orig_recvfrom(fd(), buf, len, flags, src_addr, addrlen);
    if (ret != -1) {
        event_handler_->AfterRead(buf, ret);
    }
    return ret;
}

ssize_t TracingSocket::Recv(void *buf, size_t len, int flags) {
    event_handler_->BeforeRead();
    ssize_t ret = orig().orig_recv(fd(), buf, len, flags);
    if (ret != -1) {
        event_handler_->AfterRead(buf, ret);
    }
    return ret;
}

ssize_t TracingSocket::Read(void *buf, size_t count) {
    event_handler_->BeforeRead();
    ssize_t ret = orig().orig_read(fd(), buf, count);
    if (ret != -1) {
        event_handler_->AfterRead(buf, ret);
    }
    return ret;
}

ssize_t TracingSocket::Send(const void *buf, size_t len, int flags) {
    event_handler_->BeforeWrite();
    ssize_t ret = orig().orig_send(fd(), buf, len, flags);
    if (ret != -1) {
        event_handler_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    }
    return ret;
}

ssize_t TracingSocket::Write(const void *buf, size_t count) {
    event_handler_->BeforeWrite();
    ssize_t ret = orig().orig_write(fd(), buf, count);
    if (ret != -1) {
        event_handler_->AfterWrite(set_iovec(buf, count), SINGLE_IOVEC, ret);
    }
    return ret;
}

ssize_t TracingSocket::Writev(const struct iovec *iov, int iovcnt) {
    event_handler_->BeforeWrite();
    ssize_t ret = orig().orig_writev(fd(), iov, iovcnt);
    if (ret != -1) {
        event_handler_->AfterWrite(iov, iovcnt, ret);
    }
    return ret;
}

ssize_t TracingSocket::SendTo(int sockfd, const void *buf, size_t len,
                              int flags, const struct sockaddr *dest_addr,
                              socklen_t addrlen) {
    event_handler_->BeforeWrite();
    ssize_t ret =
        orig().orig_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    if (ret != -1) {
        event_handler_->AfterWrite(set_iovec(buf, len), SINGLE_IOVEC, ret);
    }
    return ret;
}

int TracingSocket::Close() {
    int ret = orig().orig_close(fd());
    return ret;
}
