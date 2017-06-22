#pragma once

namespace microtrace {

/*
 * SocketInterface contains the BSD Socket API methods that are used for
 * tracing.
 */
class SocketInterface {
   public:
    virtual ~SocketInterface() = default;

    virtual int fd() const = 0;

    /*
     * Calling Async() indicates that the socket will be used in asynchronous
     * mode (non-blocking). Async should be called when current context is set
     * to the context that created this socket.
    */
    virtual void Async() = 0;

    virtual ssize_t RecvFrom(void *buf, size_t len, int flags,
                             struct sockaddr *src_addr, socklen_t *addrlen) = 0;
    virtual ssize_t Recv(void *buf, size_t len, int flags) = 0;
    virtual ssize_t Read(void *buf, size_t count) = 0;
    virtual ssize_t Write(const void *buf, size_t count) = 0;
    virtual ssize_t Writev(const struct iovec *iov, int iovcnt) = 0;
    virtual ssize_t Send(const void *buf, size_t len, int flags) = 0;
    virtual ssize_t SendTo(const void *buf, size_t len, int flags,
                           const struct sockaddr *dest_addr,
                           socklen_t addrlen) = 0;
    virtual ssize_t SendMsg(const struct msghdr *msg, int flags) = 0;

    virtual int Close() = 0;
};
}
