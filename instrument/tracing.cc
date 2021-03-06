#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <unordered_map>

#include "client_socket.h"
#include "client_socket_handler.h"
#include "context.h"
#include "orig_functions.h"
#include "server_socket.h"
#include "server_socket_handler.h"
#include "socket_map.h"
#include "trace_logger.h"
#include "tracing.h"

#define SOCK_CALL(fd, traced, normal) \
    do {                              \
        auto* sock = GetSocket(fd);   \
        if (sock == nullptr) {        \
            return orig().normal;     \
        } else {                      \
            return sock->traced;      \
        }                             \
    } while (0)

using namespace microtrace;

namespace microtrace {

static const int DNS_PORT = 53;
static const int PG_PORT = 5432;

// TODO make these easy to switch
static std::shared_ptr<TraceLogger> null_logger(new NullTraceLogger);
static std::shared_ptr<TraceLogger> stdout_logger(new StdoutTraceLogger);

template <class CbType>
struct CallbackWrap {
    CallbackWrap(void* req_ptr, uv_getaddrinfo_cb orig_cb, Context context)
        : req_ptr(req_ptr), orig_cb(orig_cb), context(context) {}

    void* const req_ptr;
    const CbType orig_cb;
    const Context context;
};

typedef CallbackWrap<uv_getaddrinfo_cb> GetAddrinfoCbWrap;

static auto& thrift_instance() {
    static ThriftLoggerInstance thrift;
    return thrift;
}

static auto& socket_map() {
    static SocketMap socket_map_;
    return socket_map_;
}

static auto& getaddrinfo_cbs() {
    static std::unordered_map<void*, std::unique_ptr<GetAddrinfoCbWrap>>
        getaddrinfo_cbs;
    return getaddrinfo_cbs;
}

static void SaveSocket(std::unique_ptr<SocketInterface> entry) {
    const int fd = entry->fd();
    socket_map().Set(fd, std::move(entry));
}

static SocketInterface* GetSocket(const int sockfd) {
    return socket_map().Get(sockfd);
}

static void DeleteSocket(const int sockfd) { socket_map().Delete(sockfd); }

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
    auto handler = std::make_unique<ServerSocketHandlerImpl>(
        sockfd, thrift_instance().get(), orig());
    auto socket =
        std::make_unique<ServerSocket>(sockfd, std::move(handler), orig());
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
        int fd = uv_fd(client);
        HandleAccept(fd);
        GetSocket(fd)->Async();
    }

    return ret;
}

int socket(int domain, int type, int protocol) {
    int sockfd = orig().socket(domain, type, protocol);
    if (sockfd == -1) {
        return sockfd;
    }

    if (domain == AF_UNIX || domain == AF_LOCAL) {
        return sockfd;
    }
    // We only track IP sockets
    if (!(domain == AF_INET || domain == AF_INET6)) {
        return sockfd;
    }

    auto handler = std::make_unique<ClientSocketHandlerImpl>(
        sockfd, thrift_instance().get(), orig());
    auto socket =
        std::make_unique<ClientSocket>(sockfd, std::move(handler), orig());
    SaveSocket(std::move(socket));

    return sockfd;
}

void HandleConnect(SocketInterface* sock, const struct sockaddr* addr) {
    std::string ip;
    if (addr->sa_family == AF_INET) {
        char* ip_buf =
            inet_ntoa(reinterpret_cast<const sockaddr_in*>(addr)->sin_addr);
        ip = std::string{ip_buf, strlen(ip_buf)};
    } else if (addr->sa_family == AF_INET6) {
        char ip_buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6,
                  &(reinterpret_cast<const sockaddr_in6*>(addr)->sin6_addr),
                  ip_buf, INET6_ADDRSTRLEN);
        ip = std::string{ip_buf, strlen(ip_buf)};
    } else {
        // Assume its AF_LOCAL
        ip = "localhost";
    }
    static_cast<ClientSocket*>(sock)->Connected(ip);
}

/*
 * We use connect to fiter out sockets that we are not interested in.
 */
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    const int port = get_port(addr);

    // Don't trace connections to the collector
    // TODO also check that it's localhost
    if (port == ThriftLogger::COLLECTOR_PORT) {
        DeleteSocket(sockfd);
    }

    // We don't want to trace DNS requests, instead, we use uv_getaddrinfo keep
    // track of the context in libuv, otherwise lookups are blocking
    if (port == DNS_PORT) {
        DeleteSocket(sockfd);
    }

    // We don't trace Postgres sockets, it is traced through the Postgres client
    // library functions
    if (port == PG_PORT) {
        DeleteSocket(sockfd);
    }

    int ret = orig().connect(sockfd, addr, addrlen);

    auto* sock = GetSocket(sockfd);
    if (sock) {
        HandleConnect(sock, addr);
    }

    return ret;
}

/*
 * uv_tcp_connect will eventually call connect(), so we only add tracing that is
 * specific to uv_tcp_connect(), but not connect().
 */
int uv_tcp_connect(uv_connect_t* req, uv_tcp_t* handle,
                   const struct sockaddr* addr, uv_connect_cb cb) {
    int ret = orig().uv_tcp_connect(req, handle, addr, cb);

    auto* sock = GetSocket(uv_fd(handle));
    if (ret == 0 && sock) {
        sock->Async();
        // TODO resolve this, it should always call connect so this should not
        // be necessary
        HandleConnect(sock, addr);
    }
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
    auto* sock = GetSocket(fd);
    if (sock) {
        // IMPORTANT: do this before executing close, because it might get
        // interrupted
        DeleteSocket(fd);
    }

    return orig().close(fd);
}

typedef PGresult* (*pg_t)(PGconn* conn, const char* command);

pg_t pg() {
    static thread_local pg_t fun = nullptr;
    if (fun == nullptr) {
        void* handle = dlopen(
            "/usr/local/lib/python2.7/site-packages/psycopg2/.libs/"
            "libpq-9c51d239.so.5.9",
            RTLD_NOW);
        fun = (pg_t)dlsym(handle, "PQexec");
        dlclose(handle);
    }
    return fun;
}

PGresult* PQexec(PGconn* conn, const char* command) {
    if (!is_context_undefined() && !get_current_context().is_zero()) {
        Context context = get_current_context();

        proto::RequestLog log;
        proto::Connection* log_conn = log.mutable_conn();
        log_conn->set_server_hostname("Postgres Database");
        log_conn->set_client_hostname(GetHostname());

        proto::Context* ctx = log.mutable_context();
        ctx->mutable_trace_id()->set_high(context.trace().high());
        ctx->mutable_trace_id()->set_low(context.trace().low());
        ctx->mutable_span_id()->set_high(context.span().high());
        ctx->mutable_span_id()->set_low(context.span().low());
        ctx->mutable_parent_span()->set_high(context.parent_span().high());
        ctx->mutable_parent_span()->set_low(context.parent_span().low());

        log.set_transaction_count(1);
        log.set_role(proto::RequestLog::CLIENT);
        log.set_info("SQL: " + std::string{command, strlen(command)});

        Transaction txn;

        txn.Start();
        PGresult* res = pg()(conn, command);
        txn.End();

        log.set_time(txn.start());
        log.set_duration(txn.duration());
        thrift_instance().get()->Log(log);

        context.NewSpan();
        set_current_context(context);

        return res;
    } else {
        return pg()(conn, command);
    }
}
