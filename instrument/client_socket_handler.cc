#include "client_socket_handler.h"

#include <chrono>
#include <iostream>

#include "spdlog/spdlog.h"

#include "common.h"
#include "orig_functions.h"

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

ClientSocketHandler::ClientSocketHandler(int sockfd, TraceLogger* trace_logger,
                                         const OriginalFunctions& orig)
    : AbstractSocketHandler(sockfd, SocketRole::CLIENT, SocketState::WILL_WRITE,
                            orig),
      txn_(nullptr),
      trace_logger_(trace_logger) {}

void ClientSocketHandler::FillRequestLog(RequestLogWrapper& log) {
    VERIFY(conn_init_ == true,
           "FillRequestLog was called when conn_init is false");

    proto::Connection* conn = log->mutable_conn();
    conn->set_allocated_server_ip(&conn_.server_ip);
    conn->set_server_port(conn_.server_port);
    conn->set_allocated_client_ip(&conn_.client_ip);
    conn->set_client_port(conn_.client_port);

    proto::Context* ctx = log->mutable_context();
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
    type_ = SocketType::ASYNC;
    context_.reset(new Context(get_current_context()));
}

SocketAction ClientSocketHandler::get_next_action(
    const SocketOperation op) const {
    if (op == SocketOperation::WRITE) {
        if (state_ == SocketState::WILL_WRITE || state_ == SocketState::READ) {
            return SocketAction::SEND_REQUEST;
        }
    } else if (op == SocketOperation::READ) {
        if (state_ == SocketState::WILL_READ || state_ == SocketState::WROTE) {
            return SocketAction::RECV_RESPONSE;
        }
    }
    return SocketAction::NONE;
}

bool ClientSocketHandler::SendContextBlocking() {
    // This should succeed at first - the send buffer is empty at this point
    auto ret =
        orig_.write(fd(), reinterpret_cast<const void*>(&context().storage()),
                    sizeof(ContextStorage));
    VERIFY(ret == sizeof(ContextStorage), "Could not send context in one send");
    return true;
}

// TODO implement properly
bool ClientSocketHandler::SendContextAsync() {
    auto ret =
        orig_.write(fd(), reinterpret_cast<const void*>(&context().storage()),
                    sizeof(ContextStorage));
    VERIFY(ret == sizeof(ContextStorage), "yolo");
    return true;
}

bool ClientSocketHandler::SendContext() {
    bool result;
    if (type_ == SocketType::ASYNC) {
        result = SendContextAsync();
    } else {
        result = SendContextBlocking();
    }

    if (result) {
        context_processed_ = true;
    }
    return result;
}

bool ClientSocketHandler::SendContextIfNecessary() {
    if (!is_context_processed() &&
        get_next_action(SocketOperation::WRITE) == SocketAction::SEND_REQUEST) {
        return SendContext();
    }
    return true;
}

SocketHandler::Result ClientSocketHandler::BeforeWrite(const struct iovec* iov,
                                                       int iovcnt) {
    // New transaction
    if (get_next_action(SocketOperation::WRITE) == SocketAction::SEND_REQUEST) {
        // Only copy current context if it is a blocking socket, because the
        // socket might be in a connection pool. Since in threaded servers, one
        // thread handles a single user request, current context is what we
        // need.
        if (type_ == SocketType::BLOCKING) {
            context_.reset(new Context(get_current_context()));
        }
    }

    set_current_context(context());
    VERIFY(SendContextIfNecessary(), "Could not send context");

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

    if (get_next_action(SocketOperation::WRITE) == SocketAction::SEND_REQUEST) {
        txn_.reset(new Transaction);
        txn_->Start();
        ++num_transactions_;
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
    if (get_next_action(SocketOperation::READ) == SocketAction::RECV_RESPONSE) {
        set_current_context(context());

        txn_->End();
        {
            RequestLogWrapper log;
            FillRequestLog(log);
            trace_logger_->Log(log.get());
        }

        // Start new span only after we recevied the response
        context_->NewSpan();
    }

    // After a read is successfully executed, we set context_processed to
    // false, so at the next write it will be sent
    context_processed_ = false;

    state_ = SocketState::READ;
}

SocketHandler::Result ClientSocketHandler::BeforeClose() { return Result::Ok; }

void ClientSocketHandler::AfterClose(int ret) {}
}
