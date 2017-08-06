#include "server_socket.h"

#include <assert.h>
#include <errno.h>
#include <iostream>

#include "orig_functions.h"
#include "request_log.pb.h"

namespace microtrace {

ServerSocket::ServerSocket(const int fd,
                           std::unique_ptr<ServerSocketHandler> handler,
                           const OriginalFunctions &orig)
    : InstrumentedSocket(fd, orig),
      handler_(std::move(handler)),
      ctx_buf_start_(0) {
    VERIFY(fd_ == handler_->fd(),
           "handler and underlying socket's fd is not the same");
}

void ServerSocket::Async() { handler_->Async(); }

ssize_t ServerSocket::ReadContextBlocking() {
    // Since we are a blocking socket, we call read until the whole context is
    // read. We only stop if read returns an error, if that happens, reading can
    // be resumed later.
    while (ctx_buf_start_ != ctx_buf_.size()) {
        auto ret = orig_.read(fd(), ctx_buf_.data() + ctx_buf_start_,
                              ctx_buf_.size() - ctx_buf_start_);
        if (ret <= 0) {
            return ret;
        }
        ctx_buf_start_ += ret;
    }

    VERIFY(ctx_buf_start_ == ctx_buf_.size(), "Context could not be read");
    ctx_buf_start_ = 0;  // reset start

    ContextStorage context_storage;
    memcpy(&context_storage, ctx_buf_.data(), ctx_buf_.size());

    // Pass context to handler
    handler_->ContextReadCallback(
        std::make_unique<Context>(std::move(context_storage)));

    return ctx_buf_.size();
}

ssize_t ServerSocket::ReadContextAsync() {
    // We only do 1 non-blocking read
    auto ret = orig_.read(fd(), ctx_buf_.data() + ctx_buf_start_,
                          ctx_buf_.size() - ctx_buf_start_);
    if (ret <= 0) {
        return ret;
    }

    ctx_buf_start_ += ret;

    if (ctx_buf_start_ == ctx_buf_.size()) {
        ContextStorage context_storage;
        memcpy(&context_storage, ctx_buf_.data(), ctx_buf_.size());

        // Pass context to handler
        handler_->ContextReadCallback(
            std::make_unique<Context>(std::move(context_storage)));

        ctx_buf_start_ = 0;  // reset start
        return ctx_buf_.size();
    } else {
        // In this case we read some bytes but not the whole context, so we
        // must not let the applcation start reading from this socket, until the
        // whole context is read, so we return an EAGAIN.
        errno = EAGAIN;
        return -1;
    }
}

ssize_t ServerSocket::ReadContextIfNecessary() {
    // Frontend servers don't receive context
    if (handler_->server_type() == ServerType::FRONTEND) {
        return 1;
    }

    // We only receive context at the start of a new incoming request
    if (!handler_->is_context_processed() &&
        handler_->get_next_action(SocketOperation::READ) ==
            SocketAction::RECV_REQUEST) {
        if (handler_->type() == SocketType::BLOCKING) {
            return ReadContextBlocking();
        } else {
            return ReadContextAsync();
        }
    } else {
        return 1;
    }
}

ssize_t ServerSocket::RecvFrom(void *buf, size_t len, int flags,
                               struct sockaddr *src_addr, socklen_t *addrlen) {
    handler_->BeforeRead(buf, len);
    auto ret = ReadContextIfNecessary();
    if (ret <= 0) {
        return ret;
    }
    ret = orig_.recvfrom(fd(), buf, len, flags, src_addr, addrlen);
    handler_->AfterRead(buf, len, ret);
    return ret;
}

ssize_t ServerSocket::Recv(void *buf, size_t len, int flags) {
    handler_->BeforeRead(buf, len);
    auto ret = ReadContextIfNecessary();
    if (ret <= 0) {
        return ret;
    }
    ret = orig_.recv(fd(), buf, len, flags);
    handler_->AfterRead(buf, len, ret);
    return ret;
}

ssize_t ServerSocket::Read(void *buf, size_t count) {
    handler_->BeforeRead(buf, count);
    auto ret = ReadContextIfNecessary();
    if (ret <= 0) {
        return ret;
    }
    ret = orig_.read(fd(), buf, count);
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
