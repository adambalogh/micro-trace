#pragma once

#include <sys/socket.h>
#include <memory>

#include "common.h"
#include "socket_event_handler.h"
#include "socket_interface.h"

struct OriginalFunctions;

/*
 * A TracingSocket is a wrapper around a regular socket.
 *
 * Because of Connection: Keep-Alive, a socket may be reused for several
 * request-reply sequences, therefore a pair of sockets cannot uniquely
 * idenfity a trace.
 *
 * The assigned socket_entry must be removed if the underlying socket
 * is closed.
 */
class TracingSocket : public SocketInterface {
   public:
    TracingSocket(const TracingSocket &) = delete;

    TracingSocket(const int fd,
                  std::unique_ptr<SocketEventHandler> event_handler,
                  const OriginalFunctions &orig);

    void Accept();
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

    // File descriptor
    int fd_;

    std::unique_ptr<SocketEventHandler> event_handler_;

    // Cache for iovec struct
    struct iovec iov;

    const OriginalFunctions &orig_;
};
