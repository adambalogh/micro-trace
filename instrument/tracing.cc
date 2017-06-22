#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <chrono>
#include <memory>
#include <mutex>
#include <random>
#include <unordered_map>

#include "client_socket_handler.h"
#include "context.h"
#include "instrumented_socket.h"
#include "orig_functions.h"
#include "server_socket_handler.h"
#include "tracing.h"

using namespace microtrace;

#define SOCK_CALL(fd, traced, normal) \
    do {                              \
        auto* sock = GetSocket(fd);   \
        if (sock == nullptr) {        \
            return orig().normal;     \
        } else {                      \
            return sock->traced;      \
        }                             \
    } while (0)

namespace microtrace {

static const int DNS_PORT = 53;

template <class CbType>
struct CallbackWrap {
    CallbackWrap(void* req_ptr, uv_getaddrinfo_cb orig_cb, Context context)
        : req_ptr(req_ptr), orig_cb(orig_cb), context(context) {}

    void* const req_ptr;
    const CbType orig_cb;
    const Context context;
};

typedef CallbackWrap<uv_getaddrinfo_cb> GetAddrinfoCbWrap;

static std::mutex socket_map_mu;

static auto& socket_map() {
    static std::unordered_map<int, std::unique_ptr<SocketInterface>> socket_map;
    return socket_map;
}

static auto& getaddrinfo_cbs() {
    static std::unordered_map<void*, std::unique_ptr<GetAddrinfoCbWrap>>
        getaddrinfo_cbs;
    return getaddrinfo_cbs;
}

static void SaveSocket(std::unique_ptr<SocketInterface> entry) {
    std::unique_lock<std::mutex> l(socket_map_mu);
    LOG_ERROR_IF(socket_map().count(entry->fd()), "Socket created twice");
    socket_map()[entry->fd()] = std::move(entry);
}

static SocketInterface* GetSocket(const int sockfd) {
    std::unique_lock<std::mutex> l(socket_map_mu);
    auto it = socket_map().find(sockfd);
    if (it == socket_map().end()) {
        return nullptr;
    }
    return it->second.get();
}

static void DeleteSocket(const int sockfd) {
    std::unique_lock<std::mutex> l(socket_map_mu);
    socket_map().erase(sockfd);
}

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

    auto handler = std::make_unique<ServerSocketHandler>(sockfd);
    auto socket = std::make_unique<InstrumentedSocket>(
        sockfd, std::move(handler), orig());
    SaveSocket(std::move(socket));
}
}  // namespace microtrace

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

int socket(int domain, int type, int protocol) {
    int sockfd = orig().socket(domain, type, protocol);
    if (sockfd == -1) {
        return sockfd;
    }
    // We only track IP sockets
    if (!(domain == AF_INET || domain == AF_INET6)) {
        return sockfd;
    }

    auto handler = std::make_unique<ClientSocketHandler>(sockfd);
    auto socket = std::make_unique<InstrumentedSocket>(
        sockfd, std::move(handler), orig());
    SaveSocket(std::move(socket));

    return sockfd;
}

/*
 * We use connect to fiter out sockets that we are not interested in.
 */
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    // We don't want to trace DNS requests, instead, we use uv_getaddrinfo keep
    // track of the context
    if (get_port(addr) == DNS_PORT) {
        DeleteSocket(sockfd);
    }

    int ret = orig().connect(sockfd, addr, addrlen);
    return ret;
}

/*
 * uv_tcp_connect will eventually call connect(), so we only add tracing that is
 * specific to uv_tcp_connect(), but not connect().
 */
int uv_tcp_connect(uv_connect_t* req, uv_tcp_t* handle,
                   const struct sockaddr* addr, uv_connect_cb cb) {
    int ret = orig().uv_tcp_connect(req, handle, addr, cb);
    return ret;
}

/* uv_getaddrinfo */

void unwrap_getaddrinfo(uv_getaddrinfo_t* req, int status,
                        struct addrinfo* res) {
    const GetAddrinfoCbWrap& cb_wrap = GetGetAddrinfoCb(req);

    set_current_context(cb_wrap.context);

    uv_getaddrinfo_cb orig_cb = cb_wrap.orig_cb;
    DeleteGetAddrinfoCb(cb_wrap.req_ptr);
    orig_cb(req, status, res);
}

int uv_getaddrinfo(uv_loop_t* loop, uv_getaddrinfo_t* req,
                   uv_getaddrinfo_cb getaddrinfo_cb, const char* node,
                   const char* service, const struct addrinfo* hints) {
    if (is_context_undefined()) {
        return orig().uv_getaddrinfo(loop, req, getaddrinfo_cb, node, service,
                                     hints);
    }

    auto cb_wrap = std::make_unique<GetAddrinfoCbWrap>(req, getaddrinfo_cb,
                                                       get_current_context());
    SaveGetAddrinfoCb(std::move(cb_wrap));
    return orig().uv_getaddrinfo(loop, req, &unwrap_getaddrinfo, node, service,
                                 hints);
}

/* SocketInterface calls */

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

ssize_t sendmsg(int sockfd, const struct msghdr* msg, int flags) {
    SOCK_CALL(sockfd, SendMsg(msg, flags), sendmsg(sockfd, msg, flags));
}

int close(int fd) {
    SocketInterface* sock = GetSocket(fd);
    if (sock == NULL) {
        return orig().close(fd);
    }
    int ret = sock->Close();
    if (ret == 0) {
        DeleteSocket(fd);
    }
    return ret;
}
