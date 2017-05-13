#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <chrono>
#include <random>

#include "http_parser.h"

#include "trace.h"
#include "tracing_socket.h"

#define SOCK_CALL(fd, traced, normal)               \
    do {                                            \
        TracingSocket* sock = get_socket_entry(fd); \
        if (sock == NULL) {                         \
            return orig().normal;                   \
        } else {                                    \
            return sock->traced;                    \
        }                                           \
    } while (0)

static auto& socket_map() {
    static std::unordered_map<int, std::unique_ptr<TracingSocket>> socket_map;
    return socket_map;
}

static auto& trace_wraps() {
    static std::unordered_map<void*, std::unique_ptr<TraceWrap>> trace_wraps;
    return trace_wraps;
}

// TODO make these thread safe
void add_socket_entry(std::unique_ptr<TracingSocket> entry) {
    socket_map()[entry->fd()] = std::move(entry);
}

TracingSocket* get_socket_entry(const int sockfd) {
    auto it = socket_map().find(sockfd);
    if (it == socket_map().end()) {
        return nullptr;
    }
    return it->second.get();
}

trace_id_t get_socket_trace(const int sockfd) {
    const TracingSocket* entry = get_socket_entry(sockfd);
    if (entry == nullptr) {
        return UNDEFINED_TRACE;
    }
    return entry->fd();
}

void del_socket_entry(const int sockfd) { socket_map().erase(sockfd); }

void add_trace_wrap(std::unique_ptr<TraceWrap> trace) {
    trace_wraps()[trace->req_ptr] = std::move(trace);
}

const TraceWrap& get_trace_wrap(void* req_ptr) {
    return *(trace_wraps().at(req_ptr));
}

void del_trace_wrap(void* req_ptr) { trace_wraps().erase(req_ptr); }

/* Accept */

void HandleAccept(const int sockfd) {
    if (sockfd == -1) {
        return;
    }

    static std::mt19937 eng{
        std::chrono::high_resolution_clock::now().time_since_epoch().count()};

    // Trace ID is just a random number for now
    trace_id_t trace = std::uniform_int_distribution<>(1, 1000000)(eng);
    set_current_trace(trace);

    auto socket = std::make_unique<TracingSocket>(sockfd, trace,
                                                  SocketRole::SERVER, orig());
    socket->Accept();
    add_socket_entry(std::move(socket));

    DLOG("accepted socket: %d", sockfd);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int ret = orig().orig_accept(sockfd, addr, addrlen);
    HandleAccept(ret);

    return ret;
}

int accept4(int sockfd, struct sockaddr* addr, socklen_t* addrlen, int flags) {
    int ret = orig().orig_accept4(sockfd, addr, addrlen, flags);
    HandleAccept(ret);

    return ret;
}

int uv_accept(uv_stream_t* server, uv_stream_t* client) {
    int ret = orig().orig_uv_accept(server, client);
    if (ret == 0) {
        int fd = client->io_watcher.fd;
        HandleAccept(fd);
    }

    return ret;
}

// TODO should probably do this at connect() instead to avoid
// tagging sockets that don't communicate with other servers
int socket(int domain, int type, int protocol) {
    int sockfd = orig().orig_socket(domain, type, protocol);
    if (sockfd == -1) {
        return sockfd;
    }

    if (get_current_trace() != UNDEFINED_TRACE) {
        auto socket = std::make_unique<TracingSocket>(
            sockfd, get_current_trace(), SocketRole::CLIENT, orig());
        add_socket_entry(std::move(socket));
        DLOG("opened socket: %d", sockfd);
    }
    return sockfd;
}

/* uv_getaddrinfo */

void unwrap_getaddrinfo(uv_getaddrinfo_t* req, int status,
                        struct addrinfo* res) {
    const TraceWrap& trace = get_trace_wrap(req);

    // TODO uv_getaddrinfo might be called before a socket is opened
    assert(valid_trace(trace.id));
    set_current_trace(trace.id);

    uv_getaddrinfo_cb orig_cb = trace.orig_cb;
    del_trace_wrap(trace.req_ptr);
    orig_cb(req, status, res);
}

int uv_getaddrinfo(uv_loop_t* loop, uv_getaddrinfo_t* req,
                   uv_getaddrinfo_cb getaddrinfo_cb, const char* node,
                   const char* service, const struct addrinfo* hints) {
    std::unique_ptr<TraceWrap> trace(
        new TraceWrap(req, getaddrinfo_cb, get_current_trace()));
    add_trace_wrap(std::move(trace));

    return orig().orig_uv_getaddrinfo(loop, req, &unwrap_getaddrinfo, node,
                                      service, hints);
}

/* TracingSocket calls */

ssize_t read(int fd, void* buf, size_t count) {
    SOCK_CALL(fd, Read(buf, count), orig_read(fd, buf, count));
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
    SOCK_CALL(sockfd, Recv(buf, len, flags),
              orig_recv(sockfd, buf, len, flags));
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags,
                 struct sockaddr* src_addr, socklen_t* addrlen) {
    SOCK_CALL(sockfd, RecvFrom(buf, len, flags, src_addr, addrlen),
              orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen));
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    SOCK_CALL(fd, Writev(iov, iovcnt), orig_writev(fd, iov, iovcnt));
}

ssize_t write(int fd, const void* buf, size_t count) {
    SOCK_CALL(fd, Write(buf, count), orig_write(fd, buf, count));
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
    SOCK_CALL(sockfd, Send(buf, len, flags),
              orig_send(sockfd, buf, len, flags));
}

ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* dest_addr, socklen_t addrlen) {
    SOCK_CALL(sockfd, SendTo(buf, len, flags, dest_addr, addrlen),
              orig_sendto(sockfd, buf, len, flags, dest_addr, addrlen));
}

int close(int fd) {
    TracingSocket* sock = get_socket_entry(fd);
    if (sock == NULL) {
        return orig().orig_close(fd);
    }

    int ret = sock->Close();
    if (ret == 0) {
        del_socket_entry(fd);
    }
    return ret;
}
