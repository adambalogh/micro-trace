#pragma once

#include <sys/socket.h>
#include <functional>
#include <memory>

#include "common.h"
#include "socket_handler.h"
#include "socket_interface.h"

namespace microtrace {

struct OriginalFunctions;

typedef std::function<ssize_t()> IoFunction;

/*
 * A InstrumentedSocket is a wrapper around a regular socket, with callbacks
 * that allow tracking the socket's lifecycle.
 *
 * It must not alter the behaviour of the socket.
 */
class InstrumentedSocket : public SocketInterface {
   public:
    InstrumentedSocket(const InstrumentedSocket &) = delete;

    InstrumentedSocket(const int fd, std::unique_ptr<SocketHandler> handler,
                       const OriginalFunctions &orig);

    void Async() override;

    ssize_t RecvFrom(void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen) override;
    ssize_t Recv(void *buf, size_t len, int flags) override;
    ssize_t Read(void *buf, size_t count) override;
    ssize_t Write(const void *buf, size_t count) override;
    ssize_t Writev(const struct iovec *iov, int iovcnt) override;
    ssize_t Send(const void *buf, size_t len, int flags) override;
    ssize_t SendTo(const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr,
                   socklen_t addrlen) override;
    ssize_t SendMsg(const struct msghdr *msg, int flags) override;

    int Close() override;

    int fd() const override { return fd_; }

   private:
    struct iovec *set_iovec(const void *buf, size_t len) {
        iov.iov_base = const_cast<void *>(buf);
        iov.iov_len = len;
        return &iov;
    }

    const int fd_;

    const std::unique_ptr<SocketHandler> handler_;

    /*
     * Cache for iovec struct
     */
    struct iovec iov;

    const OriginalFunctions &orig_;
};
}
