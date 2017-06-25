#include "client_socket_handler.h"

#include <chrono>

#include "spdlog/spdlog.h"

#include "common.h"
#include "request_logger.h"

namespace microtrace {

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

ClientSocketHandler::ClientSocketHandler(int sockfd,
                                         RequestLogger* request_logger)
    : AbstractSocketHandler(sockfd, SocketRole::CLIENT,
                            SocketState::WILL_WRITE),
      txn_(nullptr),
      socket_type_(SocketType::BLOCKING),
      request_logger_(request_logger) {}

void ClientSocketHandler::FillRequestLog(RequestLogWrapper& log) {
    VERIFY(conn_init_ == true,
           "FillRequestLog was called when conn_init is false");

    proto::Connection* conn = log->mutable_conn();
    conn->set_allocated_server_ip(&conn_.server_ip);
    conn->set_server_port(conn_.server_port);
    conn->set_allocated_client_ip(&conn_.client_ip);
    conn->set_client_port(conn_.client_port);

    proto::RequestLog::Context* ctx = log->mutable_context();
    ctx->mutable_trace_id()->set_high(context().trace().high());
    ctx->mutable_trace_id()->set_low(context().trace().low());
    ctx->mutable_span_id()->set_high(context().span().high());
    ctx->mutable_span_id()->set_low(context().span().low());
    ctx->mutable_parent_span()->set_high(context().parent_span().high());
    ctx->mutable_parent_span()->set_low(context().parent_span().low());

    log->set_time(txn_->start());
    log->set_duration(txn_->duration());
    log->set_transaction_count(num_transactions_);
    log->set_role(proto::RequestLog::CLIENT);
}

void ClientSocketHandler::Async() {
    socket_type_ = SocketType::ASYNC;
    context_.reset(new Context(get_current_context()));
}

SocketHandler::Result ClientSocketHandler::BeforeWrite(const struct iovec* iov,
                                                       int iovcnt) {
    return Result::Ok;
}

void ClientSocketHandler::AfterWrite(const struct iovec* iov, int iovcnt,
                                     ssize_t ret) {
    if (ret == -1 || ret == 0) {
        return;
    }

    VERIFY(ret > 0, "write invalid return value");

    if (!conn_init_) {
        SetConnection();
    }

    // New transaction
    if (state_ == SocketState::WILL_WRITE || state_ == SocketState::READ) {
        // Only copy context if it is a blocking socket
        if (socket_type_ == SocketType::BLOCKING) {
            context_.reset(new Context(get_current_context()));
        }
        set_current_context(context());

        txn_.reset(new Transaction);
        txn_->Start();

        ++num_transactions_;
    }
    // Continue sending request
    else if (state_ == SocketState::WROTE) {
        set_current_context(context());
    }

    state_ = SocketState::WROTE;
}

SocketHandler::Result ClientSocketHandler::BeforeRead(const void* buf,
                                                      size_t len) {
    LOG_ERROR_IF(
        state_ == SocketState::WILL_WRITE,
        "ClientSocketHandler that was expected to write, read instead");

    return Result::Ok;
}

void ClientSocketHandler::AfterRead(const void* buf, size_t len, ssize_t ret) {
    set_current_context(context());

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

    // New incoming response
    if (state_ == SocketState::WROTE) {
        context_->NewSpan();
        set_current_context(context());

        txn_->End();
        {
            RequestLogWrapper log;
            FillRequestLog(log);
            request_logger_->Log(log.get());
        }
    }

    state_ = SocketState::READ;
}

SocketHandler::Result ClientSocketHandler::BeforeClose() { return Result::Ok; }

void ClientSocketHandler::AfterClose(int ret) {}
}
