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

struct TraceWrap;

// TODO make these thread-safe
struct TraceWrap {
    TraceWrap(void *req_ptr, uv_getaddrinfo_cb orig_cb, trace_id_t id)
        : req_ptr(req_ptr), orig_cb(orig_cb), id(id) {}

    void *const req_ptr;
    const uv_getaddrinfo_cb orig_cb;
    const trace_id_t id;
};

void add_socket_entry(std::unique_ptr<TracingSocket> entry);
TracingSocket *get_socket_entry(const int sockfd);
trace_id_t get_socket_trace(const int sockfd);
void del_socket_entry(const int sockfd);

void add_trace_wrap(std::unique_ptr<TraceWrap> trace);
const TraceWrap &get_trace_wrap(void *req_ptr);
void del_trace_wrap(void *req_ptr);
