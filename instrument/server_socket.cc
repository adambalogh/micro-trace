#include "server_socket.h"

#include <assert.h>

#include "orig_functions.h"
#include "request_log.pb.h"

namespace microtrace {

ServerSocket::ServerSocket(const int fd, std::unique_ptr<SocketHandler> handler,
                           const OriginalFunctions &orig)
    : InstrumentedSocket(fd, std::move(handler), orig) {
    context_buffer.resize(CONTEXT_PREFIX_SIZE + ContextProtoSize());
    VERIFY(handler_->role_server(),
           "ServerSocket given a non-server socket handler");
}

void ServerSocket::Async() { handler_->Async(); }

ssize_t ServerSocket::FillFromContextBuffer(void *buf, size_t len) {
    char *const char_buf = static_cast<char *>(buf);
    VERIFY(len >= context_buffer.size(), "read len < context_buffer size");
    for (size_t i = 0; i < context_buffer.size(); ++i) {
        char_buf[i] = context_buffer[i];
    }
    return context_buffer.size();
}

bool ServerSocket::ReadContextIfNecessary() {
    if (handler_->get_next_action(SocketOperation::READ) !=
        SocketAction::RECV_REQUEST) {
        return true;
    }

    if (handler_->type() == SocketType::BLOCKING) {
        int max_try = 3;
        int start = 0;

        // TODO make sure errors are handled correctly here
        // TODO check prefix as we read here
        do {
            ssize_t ret = orig_.read(fd(), string_arr(context_buffer) + start,
                                     context_buffer.size() - start);
            start += ret;
            --max_try;
        } while (start != context_buffer.size() && max_try > 0);

        if (start <= 0) {
            return true;
        }
        if (start < static_cast<ssize_t>(context_buffer.size())) {
            return false;
        }

        // VERIFY(ret == static_cast<ssize_t>(context_buffer.size()),
        //       "Could not read context in one read, read {} bytes", ret);

        for (int i = 0; i < CONTEXT_PREFIX_SIZE; ++i) {
            if (context_buffer[i] != CONTEXT_PREFIX[i]) {
                return false;
            }
        }

        printf("blocking context read\n");
    } else {
        printf("non-blocking context read\n");
    }

    return true;
}

ssize_t ServerSocket::RecvFrom(void *buf, size_t len, int flags,
                               struct sockaddr *src_addr, socklen_t *addrlen) {
    handler_->BeforeRead(buf, len);
    ssize_t ret;
    if (!ReadContextIfNecessary()) {
        ret = FillFromContextBuffer(buf, len);
    } else {
        ret = orig_.recvfrom(fd(), buf, len, flags, src_addr, addrlen);
    }
    handler_->AfterRead(buf, len, ret);
    return ret;
}

ssize_t ServerSocket::Recv(void *buf, size_t len, int flags) {
    handler_->BeforeRead(buf, len);
    ssize_t ret;
    if (!ReadContextIfNecessary()) {
        ret = FillFromContextBuffer(buf, len);
    } else {
        ret = orig_.recv(fd(), buf, len, flags);
    }
    handler_->AfterRead(buf, len, ret);
    return ret;
}

ssize_t ServerSocket::Read(void *buf, size_t count) {
    handler_->BeforeRead(buf, count);
    ssize_t ret;
    if (!ReadContextIfNecessary()) {
        ret = FillFromContextBuffer(buf, count);
    } else {
        ret = orig_.read(fd(), buf, count);
    }
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
