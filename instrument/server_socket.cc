#include "server_socket.h"

#include "common.h"
#include "context.h"

namespace microtrace {

ServerSocket::ServerSocket(int sockfd)
    : AbstractInstrumentedSocket(sockfd, SocketRole::SERVER,
                                 SocketState::WILL_READ) {}

ssize_t ServerSocket::Read(const void* buf, size_t len, IoFunction fun) {
    LOG_ERROR_IF(state_ == SocketState::WILL_WRITE,
                 "ServerSocket that was expected to write, read instead");

    auto ret = fun();
    if (ret == 0) {
        // peer shutdown
        return ret;
    }
    if (ret == -1) {
        return ret;
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
    if (state_ == SocketState::READ) {
        set_current_context(context());
    }

    state_ = SocketState::READ;
    return ret;
}

ssize_t ServerSocket::Write(const struct iovec* iov, int iovcnt,
                            IoFunction fun) {
    set_current_context(context());

    auto ret = fun();
    if (ret == -1 || ret == 0) {
        return ret;
    }

    VERIFY(ret > 0, "write invalid return value");

    if (!conn_init_) {
        SetConnection();
    }

    state_ = SocketState::WROTE;
    return ret;
}

int ServerSocket::Close(CloseFunction fun) { return fun(); }
}
