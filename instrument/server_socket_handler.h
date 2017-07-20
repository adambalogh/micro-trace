#pragma once

#include "socket_handler.h"

namespace microtrace {

class ServerSocketHandler : public AbstractSocketHandler {
   public:
    ServerSocketHandler(int sockfd, const OriginalFunctions& orig);

    virtual void Async() override;

    virtual Result BeforeRead(const void* buf, size_t len) override;
    virtual void AfterRead(const void* buf, size_t len, ssize_t ret) override;

    virtual Result BeforeWrite(const struct iovec* iov, int iovcnt) override;
    virtual void AfterWrite(const struct iovec* iov, int iovcnt,
                            ssize_t ret) override;

    virtual Result BeforeClose() override;
    virtual void AfterClose(int ret) override;

    SocketAction get_next_action(const SocketOperation op) const override;

    void ContextReadCallback(std::unique_ptr<Context> c);
};
}
