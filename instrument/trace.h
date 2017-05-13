#pragma once

#include <pthread.h>
#include <stdint.h>
#include <memory>
#include <unordered_map>

#include "uv.h"

#include "common.h"
#include "orig_functions.h"
#include "tracing_socket.h"

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
}

template <class CbType>
struct CallbackWrap {
    CallbackWrap(void *req_ptr, uv_getaddrinfo_cb orig_cb, trace_id_t id)
        : req_ptr(req_ptr), orig_cb(orig_cb), id(id) {}

    void *const req_ptr;
    const CbType orig_cb;
    const trace_id_t id;
};
