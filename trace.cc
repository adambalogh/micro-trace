#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

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

/* Accept */

void handle_accept(const int sockfd) {
    if (sockfd == -1) {
        return;
    }

    // Trace ID is just a random number for now
    trace_id_t trace = rand() % 10000;
    set_current_trace(trace);

    std::unique_ptr<TracingSocket> socket(
        new TracingSocket(sockfd, trace, SocketRole::SERVER));
    int ret = socket->SetConnid();
    if (ret != 0) {
    }
    add_socket_entry(std::move(socket));

    DLOG("accepted socket: %d", sockfd);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int ret = orig().orig_accept(sockfd, addr, addrlen);
    handle_accept(ret);

    return ret;
}

int accept4(int sockfd, struct sockaddr* addr, socklen_t* addrlen, int flags) {
    int ret = orig().orig_accept4(sockfd, addr, addrlen, flags);
    handle_accept(ret);

    return ret;
}

int uv_accept(uv_stream_t* server, uv_stream_t* client) {
    int ret = orig().orig_uv_accept(server, client);
    if (ret == 0) {
        int fd = client->io_watcher.fd;
        handle_accept(fd);
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

    if (current_trace != UNDEFINED_TRACE) {
        std::unique_ptr<TracingSocket> socket(
            new TracingSocket(sockfd, current_trace, SocketRole::CLIENT));
        add_socket_entry(std::move(socket));
        DLOG("opened socket: %d", sockfd);
    }
    return sockfd;
}

/* uv_getaddrinfo */

void unwrap_getaddrinfo(uv_getaddrinfo_t* req, int status,
                        struct addrinfo* res) {
    const TraceWrap& trace = get_trace_wrap(req);
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
        new TraceWrap(req, getaddrinfo_cb, current_trace));
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
    printf("recvfrom\n");
    SOCK_CALL(sockfd, RecvFrom(buf, len, flags, src_addr, addrlen),
              orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen));
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    SOCK_CALL(fd, Writev(iov, iovcnt), orig_writev(fd, iov, iovcnt));
}

ssize_t write(int fd, const void* buf, size_t count) {
    printf("write\n");
    SOCK_CALL(fd, Write(buf, count), orig_write(fd, buf, count));
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
    SOCK_CALL(sockfd, Send(buf, len, flags),
              orig_send(sockfd, buf, len, flags));
}

ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* dest_addr, socklen_t addrlen) {
    SOCK_CALL(sockfd, SendTo(sockfd, buf, len, flags, dest_addr, addrlen),
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
