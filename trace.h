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
static auto &socket_map() {
    static std::unordered_map<int, std::unique_ptr<TracingSocket>> socket_map;
    return socket_map;
}

static auto &trace_wraps() {
    static std::unordered_map<void *, std::unique_ptr<TraceWrap>> trace_wraps;
    return trace_wraps;
}

struct TraceWrap {
    TraceWrap(void *req_ptr, uv_getaddrinfo_cb orig_cb, trace_id_t id)
        : req_ptr(req_ptr), orig_cb(orig_cb), id(id) {}

    void *const req_ptr;
    const uv_getaddrinfo_cb orig_cb;
    const trace_id_t id;
};

void add_socket_entry(std::unique_ptr<TracingSocket> entry) {
    socket_map()[entry->fd()] = std::move(entry);
}

TracingSocket *get_socket_entry(const int sockfd) {
    auto it = socket_map().find(sockfd);
    if (it == socket_map().end()) {
        return nullptr;
    }
    return it->second.get();
}

trace_id_t get_socket_trace(const int sockfd) {
    const TracingSocket *entry = get_socket_entry(sockfd);
    if (entry == nullptr) {
        return UNDEFINED_TRACE;
    }
    return entry->fd();
}

void del_socket_entry(const int sockfd) { socket_map().erase(sockfd); }

void add_trace_wrap(std::unique_ptr<TraceWrap> trace) {
    trace_wraps()[trace->req_ptr] = std::move(trace);
}

const TraceWrap &get_trace_wrap(void *req_ptr) {
    return *(trace_wraps().at(req_ptr));
}

void del_trace_wrap(void *req_ptr) { trace_wraps().erase(req_ptr); }
