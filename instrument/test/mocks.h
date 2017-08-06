#pragma once

#include "client_socket_handler.h"
#include "instrumented_socket.h"
#include "orig_functions.h"
#include "server_socket_handler.h"

using namespace microtrace;

class DumbClientSocketHandler : public ClientSocketHandler {
   public:
    DumbClientSocketHandler(int fd, const OriginalFunctions &orig)
        : ClientSocketHandler(fd, orig) {}

    void Async() {}
    void HandleConnect(const std::string &ip) {}
    bool has_txn() const { return false; }

    Result BeforeRead(const void *buf, size_t len) { return Result::Ok; }
    void AfterRead(const void *buf, size_t len, ssize_t ret) {}

    Result BeforeWrite(const struct iovec *iov, int iovcnt) {
        return Result::Ok;
    }
    void AfterWrite(const struct iovec *iov, int iovcnt, ssize_t ret) {}

    Result BeforeClose() { return Result::Ok; }
    void AfterClose(int ret) {}

    SocketState state() const { return SocketState::WILL_READ; }
    SocketAction get_next_action(const SocketOperation op) const {
        return SocketAction::NONE;
    }

    virtual SocketType type() const { return SocketType::BLOCKING; }

    bool has_context() const { return true; }
    bool is_context_processed() const { return false; }
};

class DumbServerSocketHandler : public ServerSocketHandler {
   public:
    DumbServerSocketHandler(int fd, const OriginalFunctions &orig)
        : ServerSocketHandler(fd, orig) {}

    void Async() {}
    void ContextReadCallback(std::unique_ptr<Context> c) {}

    Result BeforeRead(const void *buf, size_t len) { return Result::Ok; }
    void AfterRead(const void *buf, size_t len, ssize_t ret) {}

    Result BeforeWrite(const struct iovec *iov, int iovcnt) {
        return Result::Ok;
    }
    void AfterWrite(const struct iovec *iov, int iovcnt, ssize_t ret) {}

    Result BeforeClose() { return Result::Ok; }
    void AfterClose(int ret) {}

    SocketState state() const { return SocketState::WILL_READ; }
    SocketAction get_next_action(const SocketOperation op) const {
        return SocketAction::NONE;
    }

    virtual SocketType type() const { return SocketType::BLOCKING; }
    bool is_context_processed() const { return false; }

   private:
    Context ctx_;
};
