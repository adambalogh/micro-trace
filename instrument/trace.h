#pragma once

#include <pthread.h>
#include <stdint.h>
#include <memory>
#include <unordered_map>

#include "uv.h"

#include "common.h"
#include "orig_functions.h"
#include "tracing_socket.h"

/*
 * These are the functions that we instrument in order to trace requests.
 */
extern "C" {

int socket(int domain, int type, int protocol);
int close(int fd);

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);

ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t read(int fd, void *buf, size_t count);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);

ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);

int uv_accept(uv_stream_t *server, uv_stream_t *client);
int uv_getaddrinfo(uv_loop_t *loop, uv_getaddrinfo_t *req,
                   uv_getaddrinfo_cb getaddrinfo_cb, const char *node,
                   const char *service, const struct addrinfo *hints);
}
