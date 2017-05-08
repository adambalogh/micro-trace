#ifndef _TRACE_H_
#define _TRACE_H_

#include <pthread.h>
#include <stdint.h>
#include <memory>
#include <unordered_map>

#include "uv.h"

#include "helpers.h"
#include "orig_types.h"
#include "socket_entry.h"

extern "C" {

int socket(int domain, int type, int protocol);
int close(int fd);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
}

__thread trace_id_t current_trace = UNDEFINED_TRACE;

struct TraceWrap {
    BORROWS(void *req_ptr);
    uv_getaddrinfo_cb orig_cb;
    trace_id_t id;
};

// TODO make these thread-safe
std::unordered_map<int, std::unique_ptr<SocketEntry>> socket_map_;
std::unordered_map<void *, std::unique_ptr<TraceWrap>> trace_wraps_;

int valid_trace(const trace_id_t trace) {
    return trace != UNDEFINED_TRACE && trace != -1;
}

void set_current_trace(const trace_id_t trace) {
    if (valid_trace(trace)) {
        current_trace = trace;
    }
}

void add_socket_entry(std::unique_ptr<SocketEntry> entry) {
    socket_map_[entry->fd()] = std::move(entry);
}

SocketEntry *get_socket_entry(const int sockfd) {
    try {
        return socket_map_.at(sockfd).get();
    } catch (std::out_of_range) {
        return nullptr;
    }
}

trace_id_t get_socket_trace(const int sockfd) {
    const SocketEntry *entry = get_socket_entry(sockfd);
    if (entry == nullptr) {
        return UNDEFINED_TRACE;
    }
    return entry->fd();
}

void del_socket_entry(const int sockfd) { socket_map_.erase(sockfd); }

void add_trace_wrap(std::unique_ptr<TraceWrap> trace) {
    trace_wraps_[trace->req_ptr] = std::move(trace);
}

const TraceWrap &get_trace_wrap(void *req_ptr) {
    return *(trace_wraps_.at(req_ptr));
}

void del_trace_wrap(void *req_ptr) { trace_wraps_.erase(req_ptr); }

#endif
