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

#define SOCK_CALL(fd, traced, normal)        \
    do {                                     \
        TracingSocket* sock = GetSocket(fd); \
        if (sock == NULL) {                  \
            return orig().normal;            \
        } else {                             \
            return sock->traced;             \
        }                                    \
    } while (0)

template <class CbType>
struct CallbackWrap {
    CallbackWrap(void* req_ptr, uv_getaddrinfo_cb orig_cb, trace_id_t trace)
        : req_ptr(req_ptr), orig_cb(orig_cb), trace(trace) {}

    void* const req_ptr;
    const CbType orig_cb;
    const trace_id_t trace;
};

typedef CallbackWrap<uv_getaddrinfo_cb> GetAddrinfoCbWrap;

static auto& socket_map() {
    static std::unordered_map<int, std::unique_ptr<TracingSocket>> socket_map;
    return socket_map;
}

static auto& getaddrinfo_cbs() {
    static std::unordered_map<void*, std::unique_ptr<GetAddrinfoCbWrap>>
        getaddrinfo_cbs;
    return getaddrinfo_cbs;
}

// TODO make these thread safe
static void SaveSocket(std::unique_ptr<TracingSocket> entry) {
    socket_map()[entry->fd()] = std::move(entry);
}

static TracingSocket* GetSocket(const int sockfd) {
    auto it = socket_map().find(sockfd);
    if (it == socket_map().end()) {
        return nullptr;
    }
    return it->second.get();
}

static void DeleteSocket(const int sockfd) { socket_map().erase(sockfd); }

static void SaveGetAddrinfoCb(std::unique_ptr<GetAddrinfoCbWrap> wrap) {
    getaddrinfo_cbs()[wrap->req_ptr] = std::move(wrap);
}

static const GetAddrinfoCbWrap& GetGetAddrinfoCb(void* req_ptr) {
    return *(getaddrinfo_cbs().at(req_ptr));
}

static void DeleteGetAddrinfoCb(void* req_ptr) {
    getaddrinfo_cbs().erase(req_ptr);
}

/* Accept */

static void HandleAccept(const int sockfd) {
    if (sockfd == -1) {
        return;
    }

    static std::mt19937 eng{
        std::chrono::high_resolution_clock::now().time_since_epoch().count()};

    // Trace ID is just a random number for now
    trace_id_t trace = std::uniform_int_distribution<>(1, 1000000)(eng);
    set_current_trace(trace);

    auto event_handler =
        std::make_unique<SocketEventHandler>(sockfd, trace, SocketRole::SERVER);
    auto socket = std::make_unique<TracingSocket>(
        sockfd, std::move(event_handler), orig());
    socket->Accept();
    SaveSocket(std::move(socket));

    DLOG("accepted socket: %d", sockfd);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int ret = orig().accept(sockfd, addr, addrlen);
    HandleAccept(ret);
    return ret;
}

int accept4(int sockfd, struct sockaddr* addr, socklen_t* addrlen, int flags) {
    int ret = orig().accept4(sockfd, addr, addrlen, flags);
    HandleAccept(ret);
    return ret;
}

int uv_accept(uv_stream_t* server, uv_stream_t* client) {
    int ret = orig().uv_accept(server, client);
    if (ret == 0) {
        int fd = client->io_watcher.fd;
        HandleAccept(fd);
    }

    return ret;
}

// TODO should probably do this at connect() instead to avoid
// tagging sockets that don't communicate with other servers
int socket(int domain, int type, int protocol) {
    int sockfd = orig().socket(domain, type, protocol);
    if (sockfd == -1) {
        return sockfd;
    }

    if (get_current_trace() != UNDEFINED_TRACE) {
        auto event_handler = std::make_unique<SocketEventHandler>(
            sockfd, get_current_trace(), SocketRole::CLIENT);
        auto socket = std::make_unique<TracingSocket>(
            sockfd, std::move(event_handler), orig());
        SaveSocket(std::move(socket));
        DLOG("opened socket: %d", sockfd);
    }
    return sockfd;
}

/* uv_getaddrinfo */

void unwrap_getaddrinfo(uv_getaddrinfo_t* req, int status,
                        struct addrinfo* res) {
    const GetAddrinfoCbWrap& cb_wrap = GetGetAddrinfoCb(req);

    // TODO uv_getaddrinfo might be called before a socket is opened
    assert(valid_trace(cb_wrap.trace));
    set_current_trace(cb_wrap.trace);

    uv_getaddrinfo_cb orig_cb = cb_wrap.orig_cb;
    DeleteGetAddrinfoCb(cb_wrap.req_ptr);
    orig_cb(req, status, res);
}

int uv_getaddrinfo(uv_loop_t* loop, uv_getaddrinfo_t* req,
                   uv_getaddrinfo_cb getaddrinfo_cb, const char* node,
                   const char* service, const struct addrinfo* hints) {
    auto cb_wrap = std::make_unique<GetAddrinfoCbWrap>(req, getaddrinfo_cb,
                                                       get_current_trace());
    SaveGetAddrinfoCb(std::move(cb_wrap));
    return orig().uv_getaddrinfo(loop, req, &unwrap_getaddrinfo, node, service,
                                 hints);
}

/* TracingSocket calls */

ssize_t read(int fd, void* buf, size_t count) {
    SOCK_CALL(fd, Read(buf, count), read(fd, buf, count));
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
    SOCK_CALL(sockfd, Recv(buf, len, flags), recv(sockfd, buf, len, flags));
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags,
                 struct sockaddr* src_addr, socklen_t* addrlen) {
    SOCK_CALL(sockfd, RecvFrom(buf, len, flags, src_addr, addrlen),
              recvfrom(sockfd, buf, len, flags, src_addr, addrlen));
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    SOCK_CALL(fd, Writev(iov, iovcnt), writev(fd, iov, iovcnt));
}

ssize_t write(int fd, const void* buf, size_t count) {
    SOCK_CALL(fd, Write(buf, count), write(fd, buf, count));
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
    SOCK_CALL(sockfd, Send(buf, len, flags), send(sockfd, buf, len, flags));
}

ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* dest_addr, socklen_t addrlen) {
    SOCK_CALL(sockfd, SendTo(buf, len, flags, dest_addr, addrlen),
              sendto(sockfd, buf, len, flags, dest_addr, addrlen));
}

int close(int fd) {
    TracingSocket* sock = GetSocket(fd);
    if (sock == NULL) {
        return orig().close(fd);
    }

    int ret = sock->Close();
    if (ret == 0) {
        DeleteSocket(fd);
    }
    return ret;
}
