#pragma once

#include <sys/socket.h>
#include <memory>

#include "common.h"
#include "socket_callback.h"
#include "socket_interface.h"

struct OriginalFunctions;

/*
 * A InstrumentedSocket is a wrapper around a regular socket, with callbacks
 * that allow tracking the socket's lifecycle.
 *
 * It must not alter the behaviour of the socket.
 */
class InstrumentedSocket : public SocketInterface {
   public:
    InstrumentedSocket(const InstrumentedSocket &) = delete;

    InstrumentedSocket(const int fd,
                       std::unique_ptr<SocketCallback> event_handler,
                       const OriginalFunctions &orig);

    void Accept() override;
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
    int Close() override;

    int fd() const override { return fd_; }

   private:
    struct iovec *set_iovec(const void *buf, size_t len) {
        iov.iov_base = const_cast<void *>(buf);
        iov.iov_len = len;
        return &iov;
    }

    const int fd_;

    const std::unique_ptr<SocketCallback> callback_;

    /*
     * Cache for iovec struct
     */
    struct iovec iov;

    const OriginalFunctions &orig_;
};
