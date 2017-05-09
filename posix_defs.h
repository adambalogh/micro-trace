#pragma once

#include <dlfcn.h>

#include "uv.h"

#define ORIG(func, name)                        \
    do {                                        \
        func = (decltype(func))find_orig(name); \
    } while (0)

inline void *find_orig(const char *name) { return dlsym(RTLD_NEXT, name); }

/* Libc functions */

typedef int (*orig_socket_t)(int domain, int type, int protocol);
typedef int (*orig_close_t)(int fd);
typedef ssize_t (*orig_recvfrom_t)(int sockfd, void *buf, size_t len, int flags,
                                   struct sockaddr *src_addr,
                                   socklen_t *addrlen);
typedef ssize_t (*orig_send_t)(int sockfd, const void *buf, size_t len,
                               int flags);
typedef int (*orig_accept_t)(int sockfd, struct sockaddr *addr,
                             socklen_t *addrlen);
typedef int (*orig_accept4_t)(int sockfd, struct sockaddr *addr,
                              socklen_t *addrlen, int flags);
typedef ssize_t (*orig_recv_t)(int sockfd, void *buf, size_t len, int flags);
typedef ssize_t (*orig_read_t)(int fd, void *buf, size_t count);
typedef ssize_t (*orig_write_t)(int fd, const void *buf, size_t count);
typedef ssize_t (*orig_writev_t)(int fd, const struct iovec *iov, int iovcnt);

/* Libuv functions */

typedef int (*orig_uv_accept_t)(uv_stream_t *server, uv_stream_t *client);
typedef int (*orig_uv_getaddrinfo_t)(uv_loop_t *loop, uv_getaddrinfo_t *req,
                                     uv_getaddrinfo_cb getaddrinfo_cb,
                                     const char *node, const char *service,
                                     const struct addrinfo *hints);

struct OriginalFunctions {
    OriginalFunctions() {
        ORIG(orig_socket, "socket");
        ORIG(orig_close, "close");
        ORIG(orig_recvfrom, "recvfrom");
        ORIG(orig_send, "send");
        ORIG(orig_accept, "accept");
        ORIG(orig_accept4, "accept4");
        ORIG(orig_recv, "recv");
        ORIG(orig_read, "read");
        ORIG(orig_write, "write");
        ORIG(orig_writev, "writev");

        ORIG(orig_uv_accept, "uv_accept");
        ORIG(orig_uv_getaddrinfo, "uv_getaddrinfo");
    }

    orig_socket_t orig_socket;
    orig_close_t orig_close;
    orig_recvfrom_t orig_recvfrom;
    orig_send_t orig_send;
    orig_accept_t orig_accept;
    orig_accept4_t orig_accept4;
    orig_recv_t orig_recv;
    orig_read_t orig_read;
    orig_write_t orig_write;
    orig_writev_t orig_writev;

    orig_uv_accept_t orig_uv_accept;
    orig_uv_getaddrinfo_t orig_uv_getaddrinfo;
};

static OriginalFunctions orig;

#undef ORIG
