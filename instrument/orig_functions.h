#pragma once

#include <dlfcn.h>

#include "uv.h"

/* Libc functions */

typedef int (*orig_socket_t)(int domain, int type, int protocol);
typedef int (*orig_close_t)(int fd);
typedef int (*orig_accept_t)(int sockfd, struct sockaddr *addr,
                             socklen_t *addrlen);
typedef int (*orig_accept4_t)(int sockfd, struct sockaddr *addr,
                              socklen_t *addrlen, int flags);
typedef ssize_t (*orig_recv_t)(int sockfd, void *buf, size_t len, int flags);
typedef ssize_t (*orig_read_t)(int fd, void *buf, size_t count);
typedef ssize_t (*orig_recvfrom_t)(int sockfd, void *buf, size_t len, int flags,
                                   struct sockaddr *src_addr,
                                   socklen_t *addrlen);
typedef ssize_t (*orig_write_t)(int fd, const void *buf, size_t count);
typedef ssize_t (*orig_writev_t)(int fd, const struct iovec *iov, int iovcnt);
typedef ssize_t (*orig_send_t)(int sockfd, const void *buf, size_t len,
                               int flags);
typedef ssize_t (*orig_sendto_t)(int sockfd, const void *buf, size_t len,
                                 int flags, const struct sockaddr *dest_addr,
                                 socklen_t addrlen);

/* Libuv functions */

typedef int (*orig_uv_accept_t)(uv_stream_t *server, uv_stream_t *client);
typedef int (*orig_uv_getaddrinfo_t)(uv_loop_t *loop, uv_getaddrinfo_t *req,
                                     uv_getaddrinfo_cb getaddrinfo_cb,
                                     const char *node, const char *service,
                                     const struct addrinfo *hints);

struct OriginalFunctions {
    OriginalFunctions();

    int socket(int domain, int type, int protocol) const {
        return orig_socket(domain, type, protocol);
    }
    int close(int fd) const { return orig_close(fd); }
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) const {
        return orig_accept(sockfd, addr, addrlen);
    }
    int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
                int flags) const {
        return orig_accept4(sockfd, addr, addrlen, flags);
    }
    ssize_t recv(int sockfd, void *buf, size_t len, int flags) const {
        return orig_recv(sockfd, buf, len, flags);
    }
    ssize_t read(int fd, void *buf, size_t count) const {
        return orig_read(fd, buf, count);
    }
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen) const {
        return orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    }
    ssize_t write(int fd, const void *buf, size_t count) const {
        return orig_write(fd, buf, count);
    }
    ssize_t writev(int fd, const struct iovec *iov, int iovcnt) const {
        return orig_writev(fd, iov, iovcnt);
    }
    ssize_t send(int sockfd, const void *buf, size_t len, int flags) const {
        return orig_send(sockfd, buf, len, flags);
    }
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen) const {
        return orig_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    }
    int uv_accept(uv_stream_t *server, uv_stream_t *client) const {
        return orig_uv_accept(server, client);
    }
    int uv_getaddrinfo(uv_loop_t *loop, uv_getaddrinfo_t *req,
                       uv_getaddrinfo_cb getaddrinfo_cb, const char *node,
                       const char *service,
                       const struct addrinfo *hints) const {
        return orig_uv_getaddrinfo(loop, req, getaddrinfo_cb, node, service,
                                   hints);
    }

   private:
    orig_socket_t orig_socket;
    orig_close_t orig_close;
    orig_recvfrom_t orig_recvfrom;
    orig_accept_t orig_accept;
    orig_accept4_t orig_accept4;
    orig_recv_t orig_recv;
    orig_read_t orig_read;
    orig_write_t orig_write;
    orig_writev_t orig_writev;
    orig_send_t orig_send;
    orig_sendto_t orig_sendto;

    orig_uv_accept_t orig_uv_accept;
    orig_uv_getaddrinfo_t orig_uv_getaddrinfo;
};

extern OriginalFunctions &orig();
