#pragma once

/*
 * SocketInterface contains the BSD Socket API methods that are used for
 * tracing.
 */
class SocketInterface {
   public:
    virtual ~SocketInterface() {}

    virtual int fd() const = 0;

    // This method is only here for managing the life-cycle of the socket, it
    // shouldn't call any Socket API methods
    virtual void Accept() = 0;
    virtual ssize_t RecvFrom(void *buf, size_t len, int flags,
                             struct sockaddr *src_addr, socklen_t *addrlen) = 0;
    virtual ssize_t Recv(void *buf, size_t len, int flags) = 0;
    virtual ssize_t Read(void *buf, size_t count) = 0;
    virtual ssize_t Write(const void *buf, size_t count) = 0;
    virtual ssize_t Writev(const struct iovec *iov, int iovcnt) = 0;
    virtual ssize_t Send(const void *buf, size_t len, int flags) = 0;
    virtual ssize_t SendTo(int sockfd, const void *buf, size_t len, int flags,
                           const struct sockaddr *dest_addr,
                           socklen_t addrlen) = 0;

    virtual int Close() = 0;
};
