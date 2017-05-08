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
#include "uthash.h"

#include "socket_entry.h"
#include "trace.h"

/* Accept */

void handle_accept(const int sockfd) {
    if (sockfd == -1) {
        return;
    }

    // Trace ID is just a random number for now
    trace_id_t trace = rand() % 10000;
    set_current_trace(trace);

    std::unique_ptr<SocketEntry> socket(
        new SocketEntry(sockfd, trace, SocketEntry::SOCKET_ACCEPTED));
    socket->SetConnid();
    add_socket_entry(std::move(socket));

    DLOG("accepted socket: %d", sockfd);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    FIND_ORIG(orig_accept, "accept");
    int ret = orig_accept(sockfd, addr, addrlen);
    handle_accept(ret);

    return ret;
}

int accept4(int sockfd, struct sockaddr* addr, socklen_t* addrlen, int flags) {
    FIND_ORIG(orig_accept4, "accept4");
    int ret = orig_accept4(sockfd, addr, addrlen, flags);
    handle_accept(ret);

    return ret;
}

int uv_accept(uv_stream_t* server, uv_stream_t* client) {
    FIND_ORIG(orig_uv_accept, "uv_accept");
    int ret = orig_uv_accept(server, client);
    if (ret == 0) {
        int fd = client->io_watcher.fd;
        handle_accept(fd);
    }

    return ret;
}

/* Read */

void handle_read(const int sockfd, const void* buf, const size_t ret) {
    SocketEntry* entry = get_socket_entry(sockfd);
    if (entry == NULL) return;

    // Set connid if it hasn't been set before, e.g. in case of
    // when a socket was opened using connect().
    if (!entry->has_connid()) {
        entry->SetConnid();
    }

    set_current_trace(entry->trace());
    DLOG("%d received %ld bytes", sockfd, ret);
}

ssize_t read(int fd, void* buf, size_t count) {
    FIND_ORIG(orig_read, "read");
    ssize_t ret = orig_read(fd, buf, count);
    if (ret == -1) {
        return ret;
    }

    handle_read(fd, buf, ret);
    return ret;
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
    FIND_ORIG(orig_recv, "recv");
    ssize_t ret = orig_recv(sockfd, buf, len, flags);
    if (ret == -1) {
        return ret;
    }

    handle_read(sockfd, buf, ret);
    return ret;
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags,
                 struct sockaddr* src_addr, socklen_t* addrlen) {
    FIND_ORIG(orig_recvfrom, "recvfrom");
    ssize_t ret = orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (ret == -1) {
        return ret;
    }

    handle_read(sockfd, buf, ret);
    return ret;
}

/* Write */

void handle_write(const int sockfd, ssize_t len) {
    SocketEntry* entry = get_socket_entry(sockfd);
    if (entry == NULL) {
        return;
    }

    // Set connid if it hasn't been set before, e.g. in case of
    // when a socket was opened using connect().
    if (!entry->has_connid()) {
        entry->SetConnid();
    }

    if (valid_trace(entry->trace())) {
        set_current_trace(entry->trace());

        // We are only interested in write to sockets that we opened to
        // other servers, aka where we act as the client
        if (entry->type_opened()) {
            LOG("sent %ld bytes", len);
        }
    }
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    FIND_ORIG(orig_writev, "writev");
    ssize_t ret = orig_writev(fd, iov, iovcnt);
    handle_write(fd, ret);
    return ret;
}

ssize_t write(int fd, const void* buf, size_t count) {
    FIND_ORIG(orig_write, "write");
    ssize_t ret = orig_write(fd, buf, count);

    handle_write(fd, ret);
    return ret;
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
    FIND_ORIG(orig_send, "send");
    ssize_t ret = orig_send(sockfd, buf, len, flags);

    handle_write(sockfd, ret);
    return ret;
}

/* Socket */

// TODO should probably do this at connect() instead to avoid
// tagging sockets that don't communicate with other servers
int socket(int domain, int type, int protocol) {
    FIND_ORIG(orig_socket, "socket");
    int sockfd = orig_socket(domain, type, protocol);
    if (sockfd == -1) {
        return sockfd;
    }

    if (current_trace != UNDEFINED_TRACE) {
        std::unique_ptr<SocketEntry> socket(
            new SocketEntry(sockfd, current_trace, SocketEntry::SOCKET_OPENED));
        add_socket_entry(std::move(socket));
        DLOG("opened socket: %d", sockfd);
    }
    return sockfd;
}

int close(int fd) {
    FIND_ORIG(orig_close, "close");
    int ret = orig_close(fd);
    if (ret == 0) {
        del_socket_entry(fd);
    }
    return ret;
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
    std::unique_ptr<TraceWrap> trace(new TraceWrap());
    trace->req_ptr = req;
    trace->orig_cb = getaddrinfo_cb;
    trace->id = current_trace;
    add_trace_wrap(std::move(trace));

    FIND_ORIG(orig_uv_getaddrinfo, "uv_getaddrinfo");

    return orig_uv_getaddrinfo(loop, req, &unwrap_getaddrinfo, node, service,
                               hints);
}
