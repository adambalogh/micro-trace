#pragma once

class SocketInterface {
   public:
    virtual ~SocketInterface() {}

    virtual ssize_t RecvFrom(int sockfd, void *buf, size_t len, int flags,
                             struct sockaddr *src_addr, socklen_t *addrlen) = 0;
    virtual ssize_t Send(int sockfd, const void *buf, size_t len,
                         int flags) = 0;
    virtual ssize_t Recv(int sockfd, void *buf, size_t len, int flags) = 0;
    virtual ssize_t Read(int fd, void *buf, size_t count) = 0;
    virtual ssize_t Write(int fd, const void *buf, size_t count) = 0;
    virtual ssize_t Writev(int fd, const struct iovec *iov, int iovcnt) = 0;
    virtual int Close() = 0;
};
