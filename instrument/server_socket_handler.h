#pragma once

#include "socket_handler.h"

namespace microtrace {

class ServerSocketHandler : public AbstractSocketHandler {
   public:
    ServerSocketHandler(int sockfd, const OriginalFunctions& orig)
        : AbstractSocketHandler(sockfd, SocketState::WILL_READ, orig) {}

    virtual void ContextReadCallback(std::unique_ptr<Context> c) = 0;
};

class ServerSocketHandlerImpl : public ServerSocketHandler {
   public:
    ServerSocketHandlerImpl(int sockfd, const OriginalFunctions& orig);

    void Async() override;

    Result BeforeRead(const void* buf, size_t len) override;
    void AfterRead(const void* buf, size_t len, ssize_t ret) override;

    Result BeforeWrite(const struct iovec* iov, int iovcnt) override;
    void AfterWrite(const struct iovec* iov, int iovcnt, ssize_t ret) override;

    Result BeforeClose() override;
    void AfterClose(int ret) override;

    SocketAction get_next_action(const SocketOperation op) const override;

    void ContextReadCallback(std::unique_ptr<Context> c) override;

   private:
    bool ShouldTrace() const;
};
}
