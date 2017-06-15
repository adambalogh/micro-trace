#include "client_socket.h"

#include <chrono>

#include "spdlog/spdlog.h"

#include "common.h"
#include "logger.h"

namespace microtrace {

// TODO make this an injectable member variable
NullLogger logger;

/*
 * Wraps a proto::RequestLog. On destruction, it releases the fields that have
 * been borrowed, and not owned by the underlying RequestLog.
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

ClientSocket::ClientSocket(int sockfd)
    : AbstractInstrumentedSocket(sockfd, SocketRole::CLIENT,
                                 SocketState::WILL_WRITE),
      txn_(nullptr) {}

void ClientSocket::FillRequestLog(RequestLogWrapper& log) {
    VERIFY(conn_init_ == true,
           "FillRequestLog was called when conn_init is false");

    proto::Connection* conn = log->mutable_conn();
    conn->set_allocated_server_ip(&conn_.server_ip);
    conn->set_server_port(conn_.server_port);
    conn->set_allocated_client_ip(&conn_.client_ip);
    conn->set_client_port(conn_.client_port);

    proto::Context* ctx = log->mutable_context();
    ctx->set_trace_id(context_->trace());
    ctx->set_span_id(context_->span());
    ctx->set_parent_span(context_->parent_span());

    log->set_time(txn_->start());
    log->set_duration(txn_->duration());
    log->set_transaction_count(num_transactions_);
    log->set_role(proto::RequestLog::CLIENT);
}

ssize_t ClientSocket::Read(const void* buf, size_t len, IoFunction fun) {
    LOG_ERROR_IF(state_ == SocketState::WILL_WRITE,
                 "ClientSocket that was expected to write, read instead");

    set_current_context(context());

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

    // New incoming response
    if (state_ == SocketState::WROTE) {
        txn_->End();

        {
            RequestLogWrapper log;
            FillRequestLog(log);
            logger.Log(log.get());
        }
        context_->NewSpan();
        set_current_context(context());
    }

    state_ = SocketState::READ;
    return ret;
}

ssize_t ClientSocket::Write(const struct iovec* iov, int iovcnt,
                            IoFunction fun) {
    auto ret = fun();
    if (ret == -1 || ret == 0) {
        return ret;
    }

    VERIFY(ret > 0, "write invalid return value");

    if (!conn_init_) {
        SetConnection();
    }

    // New transaction
    if (state_ == SocketState::WILL_WRITE || state_ == SocketState::READ) {
        context_.reset(new Context(get_current_context()));
        set_current_context(*context_);

        txn_.reset(new Transaction);
        txn_->Start();

        ++num_transactions_;
    }
    // Continue sending request
    else if (state_ == SocketState::WROTE) {
        set_current_context(context());
    }

    state_ = SocketState::WROTE;
    return ret;
}

int ClientSocket::Close(CloseFunction fun) { return fun(); }
}
