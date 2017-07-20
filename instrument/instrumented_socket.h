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

    InstrumentedSocket(const int fd, const OriginalFunctions &orig)
        : fd_(fd), orig_(orig) {}

    int fd() const override { return fd_; }

   protected:
    struct iovec *set_iovec(const void *buf, size_t len) {
        iov.iov_base = const_cast<void *>(buf);
        iov.iov_len = len;
        return &iov;
    }

    const int fd_;

    /*
     * Cache for iovec struct
     */
    struct iovec iov;

    const OriginalFunctions &orig_;
};
}
