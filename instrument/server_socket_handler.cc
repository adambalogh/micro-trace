#include "server_socket_handler.h"

#include <cstdlib>
#include <iostream>

#include "common.h"
#include "context.h"
#include "orig_functions.h"

namespace microtrace {

ServerSocketHandlerImpl::ServerSocketHandlerImpl(int sockfd,
                                                 const OriginalFunctions& orig)
    : ServerSocketHandler(sockfd, orig) {}

void ServerSocketHandlerImpl::Async() { type_ = SocketType::ASYNC; }

SocketAction ServerSocketHandlerImpl::get_next_action(
    const SocketOperation op) const {
    if (op == SocketOperation::WRITE) {
        if (state_ == SocketState::WILL_WRITE || state_ == SocketState::READ) {
            return SocketAction::SEND_RESPONSE;
        }
    } else if (op == SocketOperation::READ) {
        if (state_ == SocketState::WILL_READ || state_ == SocketState::WROTE) {
            return SocketAction::RECV_REQUEST;
        }
    }
    return SocketAction::NONE;
}

void ServerSocketHandlerImpl::ContextReadCallback(std::unique_ptr<Context> c) {
    VERIFY(c, "ContextReadCallback called with empty context");
    context_ = std::move(c);
    context_processed_ = true;
}

SocketHandler::Result ServerSocketHandlerImpl::BeforeRead(const void* buf,
                                                          size_t len) {
    LOG_ERROR_IF(
        state_ == SocketState::WILL_WRITE,
        "ServerSocketHandlerImpl that was expected to write, read instead");

    return Result::Ok;
}

bool ServerSocketHandlerImpl::ShouldTrace() const {
    // sampling every ~20th request
    return (std::rand() % 101) <= 5;
}

void ServerSocketHandlerImpl::AfterRead(const void* buf, size_t len,
                                        ssize_t ret) {
    if (ret == 0) {
        // peer shutdown
        return;
    }
    if (ret == -1) {
        return;
    }

    VERIFY(ret > 0, "read invalid return value");

    // New transaction
    if (get_next_action(SocketOperation::READ) == SocketAction::RECV_REQUEST) {
        // If we are frontend, generate a random context
        if (server_type() == ServerType::FRONTEND) {
            // We use sampling, if this request should be traced, we generate a
            // new context, otherwise we use the empty context which indicates
            // that this shouldn't be traced
            if (ShouldTrace()) {
                context_.reset(new Context);
            } else {
                context_ = Context::Zero();
            }
        }
        // Otherwise we are backend, it was passed to us by client
        // and context_ is already set through ContextReadCallback.
        else {
            VERIFY(context_, "Backend server context is empty");
        }

        context_->NewSpan();
        set_current_context(*context_);
        ++num_transactions_;
    }
    // Continue reading request
    else if (state_ == SocketState::READ) {
        set_current_context(context());
    }

    context_processed_ = false;
    state_ = SocketState::READ;
}

SocketHandler::Result ServerSocketHandlerImpl::BeforeWrite(
    const struct iovec* iov, int iovcnt) {
    set_current_context(context());
    return Result::Ok;
}

void ServerSocketHandlerImpl::AfterWrite(const struct iovec* iov, int iovcnt,
                                         ssize_t ret) {
    if (ret == -1 || ret == 0) {
        return;
    }

    VERIFY(ret > 0, "write invalid return value");

    state_ = SocketState::WROTE;
}

SocketHandler::Result ServerSocketHandlerImpl::BeforeClose() {
    return Result::Ok;
}

void ServerSocketHandlerImpl::AfterClose(int ret) {}
}
