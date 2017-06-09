#include "server_socket.h"

#include "common.h"
#include "trace.h"

namespace microtrace {

ServerSocket::ServerSocket(int sockfd)
    : AbstractInstrumentedSocket(sockfd, new_trace(), SocketRole::SERVER,
                                 SocketState::WILL_READ) {}

ssize_t ServerSocket::Read(const void* buf, size_t len, IoFunction fun) {
    LOG_ERROR_IF(console_log, state_ == SocketState::WILL_WRITE,
                 "Socket that was expected to write, read instead");

    set_current_trace(trace_);

    auto ret = fun();
    if (ret == 0) {
        // peer shutdown
        return ret;
    }
    if (ret == -1) {
        return ret;
    }

    VERIFY(console_log, ret > 0, "read invalid return value");

    if (!conn_init_) {
        SetConnection();
    }

    if (state_ == SocketState::WILL_READ || state_ == SocketState::WROTE) {
        ++num_transactions_;
        trace_ = new_trace();
        set_current_trace(trace_);
    }

    state_ = SocketState::READ;
    return ret;
}

ssize_t ServerSocket::Write(const struct iovec* iov, int iovcnt,
                            IoFunction fun) {
    set_current_trace(trace_);

    auto ret = fun();
    if (ret == -1 || ret == 0) {
        return ret;
    }

    VERIFY(console_log, ret > 0, "write invalid return value");

    if (!conn_init_) {
        SetConnection();
    }

    state_ = SocketState::WROTE;
    return ret;
}

int ServerSocket::Close(CloseFunction fun) { return fun(); }
}
