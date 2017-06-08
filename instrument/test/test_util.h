#pragma once

#include "orig_functions.h"
#include "socket_adapter.h"

using namespace microtrace;

class EmptyOriginalFunctions : public OriginalFunctions {
   public:
    int socket(int domain, int type, int protocol) const { return -1; }
    int close(int fd) const { return -1; }
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) const {
        return -1;
    }
    int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
                int flags) const {
        return -1;
    }
    ssize_t recv(int sockfd, void *buf, size_t len, int flags) const {
        return -1;
    }
    ssize_t read(int fd, void *buf, size_t count) const { return -1; }
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen) const {
        return -1;
    }
    ssize_t write(int fd, const void *buf, size_t count) const { return -1; }
    ssize_t writev(int fd, const struct iovec *iov, int iovcnt) const {
        return -1;
    }
    ssize_t send(int sockfd, const void *buf, size_t len, int flags) const {
        return -1;
    }
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen) const {
        return -1;
    }
    int uv_accept(uv_stream_t *server, uv_stream_t *client) const { return -1; }
    int uv_getaddrinfo(uv_loop_t *loop, uv_getaddrinfo_t *req,
                       uv_getaddrinfo_cb getaddrinfo_cb, const char *node,
                       const char *service,
                       const struct addrinfo *hints) const {
        return -1;
    }
};

class DumbSocket : public InstrumentedSocket {
   public:
    static std::unique_ptr<InstrumentedSocket> New(int fd,
                                                   const SocketRole role) {
        return std::make_unique<DumbSocket>(fd, role);
    }

    DumbSocket(int fd, const SocketRole role) : fd_(fd), role_(role) {}

    ssize_t Read(const void *buf, size_t len, IoFunction fun) { return fun(); }

    ssize_t Write(const struct iovec *iov, int iovcnt, IoFunction fun) {
        return fun();
    }

    int Close(CloseFunction fun) { return fun(); }

    int fd() const { return fd_; }
    SocketRole role() const { return role_; }

   private:
    int fd_;
    SocketRole role_;
};
