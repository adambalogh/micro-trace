#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>

#include "uthash.h"

#include "trace.h"

/* Accept */

void handle_accept(const int sockfd) {
    if (sockfd != -1) {
        // Trace ID is just the sockfd for now
        trace_id_t trace = rand() % 10000;

        set_trace(sockfd, trace);
        set_current_trace(trace);
    }
    DLOG("accepted socket: %d", sockfd);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    orig_accept_t orig_accept = (orig_accept_t) orig("accept");
    int ret = orig_accept(sockfd, addr, addrlen);
    handle_accept(ret);

    return ret;
}

int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    orig_accept4_t orig_accept = (orig_accept4_t) orig("accept4");
    int ret = orig_accept(sockfd, addr, addrlen, flags);
    handle_accept(ret);

    return ret;
}

int uv_accept(uv_stream_t* server, uv_stream_t* client) {
    orig_uv_accept_t orig_uv_accept = (orig_uv_accept_t) orig("uv_accept");
    int ret = orig_uv_accept(server, client);
    if (ret == 0) {
        int fd = client->io_watcher.fd;
        handle_accept(fd);
    }

    return ret;
}

/* Read */ 

void handle_read(const int sockfd, const void* buf, const size_t ret) {
    set_current_trace(get_trace(sockfd));
    DLOG("%d received %ld bytes", sockfd, ret);
}

ssize_t read(int fd, void *buf, size_t count) {
    orig_read_t orig_read = (orig_read_t) orig("read");
    ssize_t ret = orig_read(fd, buf, count);
    if (ret == -1) {
        return ret;
    }

    handle_read(fd, buf, ret);
    return ret;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    orig_recv_t orig_recv = (orig_recv_t) orig("recv");
    ssize_t ret =  orig_recv(sockfd, buf, len, flags);
    if (ret == -1) {
        return ret;
    }

    handle_read(sockfd, buf, ret);
    return ret;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    orig_recvfrom_t orig_recvfrom = (orig_recvfrom_t) orig("recvfrom");
    ssize_t ret =  orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (ret == -1) {
        return ret;
    }

    handle_read(sockfd, buf, ret);
    return ret;
}

/* Write */

void handle_write(const int sockfd, ssize_t len) {
    trace_id_t trace = get_trace(sockfd);
    if (valid_trace(trace)) {
        set_current_trace(trace);
        LOG("sent %ld bytes", len);
    }
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    orig_writev_t orig_writev = (orig_writev_t) orig("writev");
    ssize_t ret = orig_writev(fd, iov, iovcnt);
    handle_write(fd, ret);
    return ret;
}


ssize_t write(int fd, const void *buf, size_t count) {
    orig_write_t orig_write = (orig_write_t) orig("write");
    ssize_t ret = orig_write(fd, buf, count);
    handle_write(fd, ret);
    return ret;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    orig_send_t orig_send = (orig_send_t) orig("send");
    ssize_t ret = orig_send(sockfd, buf, len, flags);
    handle_write(sockfd, ret);
    return ret;
}

/* Socket */

int socket(int domain, int type, int protocol) {
    orig_socket_t orig_socket = (orig_socket_t) orig("socket");
    int ret = orig_socket(domain, type, protocol);

    if (current_trace != UNDEFINED_TRACE) {
        set_trace(ret, current_trace);
        DLOG("opened socket: %d", ret);
    }
    return ret;
}

int close(int fd) {
    orig_close_t orig_close = (orig_close_t) orig("close");
    int ret = orig_close(fd);
    if (ret == 0) {
        /*del_socket_trace(fd);*/
    }
    return ret;
}

/* uv_getaddrinfo */

void unwrap_getaddrinfo(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
    trace_wrap_t* trace = get_trace_wrap(req);
    assert(valid_trace(trace->id));
    set_current_trace(trace->id);

    uv_getaddrinfo_cb orig_cb = trace->orig_cb;
    del_trace_wrap(req);
    orig_cb(req, status, res);
}

int uv_getaddrinfo(uv_loop_t* loop, uv_getaddrinfo_t* req, uv_getaddrinfo_cb getaddrinfo_cb,
        const char* node, const char* service, const struct addrinfo* hints) {
    trace_wrap_t* trace = malloc(sizeof(trace_wrap_t));
    trace->req_ptr = req;
    trace->orig_cb = getaddrinfo_cb;
    trace->id = current_trace;
    add_trace_wrap(trace);

    orig_uv_getaddrinfo_t orig_uv_getaddrinfo =
        (orig_uv_getaddrinfo_t) orig("uv_getaddrinfo");

    return orig_uv_getaddrinfo(loop, req, &unwrap_getaddrinfo, node, service, hints);
}
