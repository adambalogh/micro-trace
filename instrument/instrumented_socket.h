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
    static const int SINGLE_IOVEC = 1;

    InstrumentedSocket(const InstrumentedSocket &) = delete;

    InstrumentedSocket(const int fd, std::unique_ptr<SocketHandler> handler,
                       const OriginalFunctions &orig)
        : fd_(fd), handler_(std::move(handler)), orig_(orig) {
        VERIFY(fd == handler_->fd(),
               "handler and underlying socket's fd is not the same");
    }

    int fd() const override { return fd_; }

   protected:
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
