#include "server_socket_handler.h"

#include "common.h"
#include "context.h"

namespace microtrace {

ServerSocketHandler::ServerSocketHandler(int sockfd)
    : AbstractSocketHandler(sockfd, SocketRole::SERVER,
                            SocketState::WILL_READ) {}

SocketHandler::Result ServerSocketHandler::BeforeRead(const void* buf,
                                                      size_t len) {
    LOG_ERROR_IF(
        state_ == SocketState::WILL_WRITE,
        "ServerSocketHandler that was expected to write, read instead");

    return Result::Ok;
}

void ServerSocketHandler::AfterRead(const void* buf, size_t len, ssize_t ret) {
    if (ret == 0) {
        // peer shutdown
        return;
    }
    if (ret == -1) {
        return;
    }

    VERIFY(ret > 0, "read invalid return value");

    if (!conn_init_) {
        SetConnection();
    }

    // New transaction
    if (state_ == SocketState::WILL_READ || state_ == SocketState::WROTE) {
        context_.reset(new Context);
        set_current_context(*context_);
        ++num_transactions_;
    }
    // Continue reading request
    else if (state_ == SocketState::READ) {
        set_current_context(context());
    }

    state_ = SocketState::READ;
}

SocketHandler::Result ServerSocketHandler::BeforeWrite(const struct iovec* iov,
                                                       int iovcnt) {
    set_current_context(context());
    return Result::Ok;
}

void ServerSocketHandler::AfterWrite(const struct iovec* iov, int iovcnt,
                                     ssize_t ret) {
    if (ret == -1 || ret == 0) {
        return;
    }

    VERIFY(ret > 0, "write invalid return value");

    if (!conn_init_) {
        SetConnection();
    }

    state_ = SocketState::WROTE;
}

SocketHandler::Result ServerSocketHandler::BeforeClose() { return Result::Ok; }

void ServerSocketHandler::AfterClose(int ret) {}
}
