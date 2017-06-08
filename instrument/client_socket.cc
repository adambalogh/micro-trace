#include "client_socket.h"

#include "spdlog/spdlog.h"

#include "common.h"
#include "logger.h"

namespace microtrace {

// TODO make this an injectable member variable
StdoutLogger logger;

/*
 * Wraps a proto::RequestLog. On destruction, it releases the fields that have
 * been borrowed, and not owned by the underlying object.
 */
struct RequestLogWrapper {
    ~RequestLogWrapper() {
        proto::Connection* conn = log.mutable_conn();
        conn->release_server_ip();
        conn->release_client_ip();
    }

    proto::RequestLog* operator->() { return &log; }

    const proto::RequestLog& get() const { return log; }

    proto::RequestLog log;
};

ClientSocket::ClientSocket(int sockfd, const trace_id_t trace)
    : AbstractInstrumentedSocket(sockfd, trace, SocketRole::CLIENT,
                                 SocketState::WILL_WRITE),
      txn_(nullptr) {}

/*
 * IMPORTANT: the string fields in RequestLog.conn are only
 * borrowed, they should be released before the request object
 * is destroyed.
 */
void ClientSocket::FillRequestLog(RequestLogWrapper& log) {
    VERIFY(console_log, conn_init_ == true,
           "FillRequestLog was called when conn_init is false");

    proto::Connection* conn = log->mutable_conn();
    conn->set_allocated_server_ip(&conn_.server_ip);
    conn->set_server_port(conn_.server_port);
    conn->set_allocated_client_ip(&conn_.client_ip);
    conn->set_client_port(conn_.client_port);

    log->set_trace_id(trace_to_string(trace_));
    // TODO use chrono to get time
    log->set_time(time(NULL));
    log->set_req_count(num_requests_);
    log->set_role(role_client() ? proto::RequestLog::CLIENT
                                : proto::RequestLog::SERVER);
}
ssize_t ClientSocket::Read(const void* buf, size_t len, IoFunction fun) {
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

    state_ = SocketState::READ;
    return ret;
}

ssize_t ClientSocket::Write(const struct iovec* iov, int iovcnt,
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

    if (state_ == SocketState::WILL_WRITE || state_ == SocketState::READ) {
        ++num_requests_;
        RequestLogWrapper log;
        FillRequestLog(log);
        logger.Log(log.get());
    }
    state_ = SocketState::WROTE;

    return ret;
}

int ClientSocket::Close(CloseFunction fun) { return fun(); }
}