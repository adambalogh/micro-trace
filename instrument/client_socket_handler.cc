#include "client_socket_handler.h"

#include <unistd.h>
#include <chrono>
#include <iostream>
#include <regex>

#include "common.h"
#include "orig_functions.h"

namespace microtrace {

ServiceIpMap ClientSocketHandlerImpl::service_map_ = ServiceIpMap{};

ServiceIpMap::ServiceIpMap() {
    int i = 0;
    std::regex re("(.+)_SERVICE_HOST=(.+)");
    std::smatch match;

    while (environ[i]) {
        std::string env_var{environ[i], strlen(environ[i])};
        try {
            if (std::regex_search(env_var, match, re)) {
                std::string service_name = match[1];
                std::string ip = match[2];
                map_.emplace(ip, service_name);
            }
        } catch (std::regex_error& e) {
            std::cout << e.what() << std::endl;
        }
        ++i;
    }
}

const std::string* ServiceIpMap::Get(const std::string& ip) {
    const auto it = map_.find(ip);
    if (it == map_.end()) {
        return nullptr;
    }
    return &(it->second);
}

ClientSocketHandlerImpl::ClientSocketHandlerImpl(int sockfd,
                                                 TraceLogger* trace_logger,
                                                 const OriginalFunctions& orig)
    : ClientSocketHandler(sockfd, orig),
      txn_(nullptr),
      trace_logger_(trace_logger),
      kubernetes_socket_(false) {
    // We can already set the hostname, it is constant
    conn_.client_hostname = GetHostname();
}

void ClientSocketHandlerImpl::Async() {
    type_ = SocketType::ASYNC;
    context_.reset(new Context(get_current_context()));
}

void ClientSocketHandlerImpl::HandleConnect(const std::string& ip) {
    auto* service_name = service_map_.Get(ip);
    if (service_name) {
        kubernetes_socket_ = true;
        conn_.server_hostname = *service_name;
    } else {
        conn_.server_hostname = ip;
    }
}

SocketAction ClientSocketHandlerImpl::get_next_action(
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

void ClientSocketHandlerImpl::FillRequestLog(RequestLogWrapper& log) {
    proto::Connection* conn = log->mutable_conn();
    conn->set_allocated_server_hostname(&conn_.server_hostname);
    conn->set_allocated_client_hostname(&conn_.client_hostname);

    proto::Context* ctx = log->mutable_context();
    ctx->mutable_trace_id()->set_high(context().trace().high());
    ctx->mutable_trace_id()->set_low(context().trace().low());
    ctx->mutable_span_id()->set_high(context().span().high());
    ctx->mutable_span_id()->set_low(context().span().low());
    ctx->mutable_parent_span()->set_high(context().parent_span().high());
    ctx->mutable_parent_span()->set_low(context().parent_span().low());

    if (http_processor_.has_url()) {
        log->set_info("HTTP: " + http_processor_.url());
    }
    log->set_time(txn_->start());
    log->set_duration(txn_->duration());
    log->set_transaction_count(num_transactions_);
    log->set_role(proto::RequestLog::CLIENT);
}

bool ClientSocketHandlerImpl::SendContextBlocking() {
    // This should succeed at first - the send buffer is empty at this point
    auto ret =
        orig_.write(fd(), reinterpret_cast<const void*>(&context().storage()),
                    sizeof(ContextStorage));
    VERIFY(ret == sizeof(ContextStorage), "Could not send context in one send");
    return true;
}

// TODO implement properly
bool ClientSocketHandlerImpl::SendContextAsync() {
    auto ret =
        orig_.write(fd(), reinterpret_cast<const void*>(&context().storage()),
                    sizeof(ContextStorage));
    VERIFY(ret == sizeof(ContextStorage),
           "Context couldn't be sent async in one send");
    return true;
}

bool ClientSocketHandlerImpl::SendContext() {
    bool result;
    if (type_ == SocketType::BLOCKING) {
        result = SendContextBlocking();
    } else {
        result = SendContextAsync();
    }

    if (result) {
        context_processed_ = true;
    }
    return result;
}

bool ClientSocketHandlerImpl::SendContextIfNecessary() {
    // Only send context if we are connected to another Kubernetes pod, and it
    // is the start of a new transaction and it hasn't been sent before.
    if (kubernetes_socket_ && !is_context_processed() &&
        get_next_action(SocketOperation::WRITE) == SocketAction::SEND_REQUEST) {
        return SendContext();
    }
    return true;
}

SocketHandler::Result ClientSocketHandlerImpl::BeforeWrite(
    const struct iovec* iov, int iovcnt) {
    // New transaction
    if (get_next_action(SocketOperation::WRITE) == SocketAction::SEND_REQUEST) {
        // Reset HTTP Processor
        http_processor_ = HttpProcessor{};

        // Only copy current context if it is a blocking socket, because the
        // socket might be in a connection pool. Since in threaded servers, one
        // thread handles a single user request, current context is what we
        // need.
        if (type_ == SocketType::BLOCKING) {
            // If the context is undefined at the moment, it means that some
            // setup communications is being done which cannot and should not be
            // attributed to user requests.
            //
            // In this case we assign a zero context to it, which means it won't
            // be traced.
            if (!is_context_undefined()) {
                context_.reset(new Context(get_current_context()));
            } else {
                context_ = Context::Zero();
            }
        }
    }

    set_current_context(context());
    VERIFY(SendContextIfNecessary(), "Could not send context");

    return Result::Ok;
}

void ClientSocketHandlerImpl::AfterWrite(const struct iovec* iov, int iovcnt,
                                         ssize_t ret) {
    if (ret == -1 || ret == 0) {
        return;
    }

    VERIFY(ret > 0, "write invalid return value");

    if (get_next_action(SocketOperation::WRITE) == SocketAction::SEND_REQUEST) {
        txn_.reset(new Transaction);
        txn_->Start();
        ++num_transactions_;
    }

    // Feed data to http parser
    for (int i = 0; i < iovcnt; ++i) {
        http_processor_.Process(
            static_cast<char*>(iov[i].iov_base),
            // do not feed data that has not been sent
            std::min(static_cast<size_t>(ret), iov[i].iov_len));
    }

    state_ = SocketState::WROTE;
}

SocketHandler::Result ClientSocketHandlerImpl::BeforeRead(const void* buf,
                                                          size_t len) {
    LOG_ERROR_IF(
        state_ == SocketState::WILL_WRITE,
        "ClientSocketHandlerImpl that was expected to write, read instead");

    return Result::Ok;
}

void ClientSocketHandlerImpl::AfterRead(const void* buf, size_t len,
                                        ssize_t ret) {
    set_current_context(context());

    if (ret == 0) {
        // peer shutdown
        return;
    }
    if (ret == -1) {
        return;
    }

    VERIFY(ret > 0, "read invalid return value");

    // New incoming response
    if (get_next_action(SocketOperation::READ) == SocketAction::RECV_RESPONSE) {
        txn_->End();
        // Log request only if we are not in an empty context, which indicates
        // that this request shouldn't be traced
        if (!context().is_zero()) {
            RequestLogWrapper log;
            FillRequestLog(log);
            trace_logger_->Log(log.get());
        }

        // Start new span after we started recieving the response
        context_->NewSpan();
        set_current_context(context());
    }

    // After a read is successfully executed, we set context_processed to
    // false, so at the next write it will be sent
    context_processed_ = false;

    state_ = SocketState::READ;
}

SocketHandler::Result ClientSocketHandlerImpl::BeforeClose() {
    return Result::Ok;
}

void ClientSocketHandlerImpl::AfterClose(int ret) {}
}
