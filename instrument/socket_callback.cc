#include "socket_callback.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include <string>

#include "request_log.pb.h"
#include "spdlog/spdlog.h"

#include "common.h"
#include "logger.h"
#include "trace.h"

// TODO make this an injectable member variable
StdoutLogger logger;

Connection::Connection()
    : client_ip('.', INET6_ADDRSTRLEN), server_ip('.', INET6_ADDRSTRLEN) {}

std::string Connection::to_string() const {
    std::string str;
    str += "[";
    str += "client: " + client_ip + ":" + std::to_string(client_port);
    str += ", ";
    str += "server: " + server_ip + ":" + std::to_string(server_port);
    str += "]";
    return str;
}

/*
 * Wraps a proto::RequestLog. On destruction, it releases the fields that have
 * been borrowed, and not owned by the underlying object.
 */
struct RequestLogWrapper {
    ~RequestLogWrapper() {
        proto::Connection* conn = log.mutable_conn();
        conn->release_server_ip();
        conn->release_client_ip();
    }

    proto::RequestLog* operator->() { return &log; }

    const proto::RequestLog& get() const { return log; }

    proto::RequestLog log;
};

void SocketCallback::AfterAccept() {
    LOG_ERROR_IF(console_log, role_ != SocketRole::SERVER,
                 "AfterAccept was called on a client role socket");
}

static unsigned short get_port(const struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ntohs(((sockaddr_in*)sa)->sin_port);
    } else {
        return ntohs(((sockaddr_in6*)sa)->sin6_port);
    }
}

void SetConnectionEndPoint(const int fd, std::string* ip, short unsigned* port,
                           int (*fn)(int, struct sockaddr*, socklen_t*)) {
    sockaddr_storage tmp_sockaddr;
    sockaddr* const sockaddr_ptr = reinterpret_cast<sockaddr*>(&tmp_sockaddr);
    socklen_t addr_len = sizeof(tmp_sockaddr);

    int ret;
    const char* dst;

    ret = fn(fd, sockaddr_ptr, &addr_len);
    // At this point, both getsockname and getpeername should
    // be successful
    VERIFY(console_log, ret == 0, "get(sock|peer)name was unsuccessful");

    *port = get_port(sockaddr_ptr);
    dst = inet_ntop(tmp_sockaddr.ss_family, sockaddr_ptr, string_arr(*ip),
                    ip->size());
    // inet_ntop should also be successful here
    VERIFY(console_log, dst == string_arr(*ip), "inet_ntop was unsuccessful");

    // inet_ntop puts a null terminated string into ip
    ip->resize(strlen(string_arr(*ip)));
}

int SocketCallback::SetConnection() {
    if (role_server()) {
        // Peer is the client
        SetConnectionEndPoint(sockfd_, &conn_.client_ip, &conn_.client_port,
                              getpeername);
        // We are the server
        SetConnectionEndPoint(sockfd_, &conn_.server_ip, &conn_.server_port,
                              getsockname);
    } else {
        // Peer is the server
        SetConnectionEndPoint(sockfd_, &conn_.server_ip, &conn_.server_port,
                              getpeername);
        // We are the client
        SetConnectionEndPoint(sockfd_, &conn_.client_ip, &conn_.client_port,
                              getsockname);
    }

    conn_init_ = true;
    return 0;
}

/*
 * IMPORTANT: the string fields in RequestLog.conn are only
 * borrowed, they should be released before the request object
 * is destroyed.
 */
void SocketCallback::FillRequestLog(RequestLogWrapper& log) {
    VERIFY(console_log, conn_init_ == true,
           "FillRequestLog was called when conn_init is false");

    proto::Connection* conn = log->mutable_conn();
    conn->set_allocated_server_ip(&conn_.server_ip);
    conn->set_server_port(conn_.server_port);
    conn->set_allocated_client_ip(&conn_.client_ip);
    conn->set_client_port(conn_.client_port);

    log->set_trace_id(trace_to_string(trace_));
    // TODO use chrono to get time
    log->set_time(time(NULL));
    log->set_req_count(num_requests_);
    log->set_role(role_client() ? proto::RequestLog::CLIENT
                                : proto::RequestLog::SERVER);
}

void SocketCallback::BeforeRead() {
    LOG_ERROR_IF(console_log, state_ == SocketState::WILL_WRITE,
                 "Socket that was expected to write, read instead");
}

void SocketCallback::AfterRead(const void* buf, ssize_t ret) {
    // We set the current trace to this socket's trace, since reading from the
    // socket means that we either received a request or a response, both of
    // which might trigger some other requests in the application. We set the
    // current trace to this socket's trace even if the connection was shut down
    // or an error occured, since this might trigger some events in the
    // application that would make requests, e.g. report error
    set_current_trace(trace_);

    if (ret == 0) {
        // peer shutdown
        return;
    }
    if (ret == -1) {
        // error
        return;
    }

    // If we get to this point, it means that the connection is open,
    // and the buffer was handed to the kernel.
    VERIFY(console_log, ret > 0, "read invalid return value");

    // At this point ret is > 0, which means that the connection is open, so
    // SetConnection should succeed.
    if (!conn_init_) {
        SetConnection();
    }

    if (role_server() &&
        (state_ == SocketState::WILL_READ || state_ == SocketState::WROTE)) {
        ++num_requests_;
        RequestLogWrapper log;
        FillRequestLog(log);
        logger.Log(log.get());
    }

    state_ = SocketState::READ;
}

void SocketCallback::BeforeWrite() {
    LOG_ERROR_IF(console_log, state_ == SocketState::WILL_READ,
                 "Socket that was expected to read, wrote instead");
}

void SocketCallback::AfterWrite(const struct iovec* iov, int iovcnt,
                                ssize_t ret) {
    // We set the current trace to this socket's trace, since writing to the
    // socket might mean that we finished writing a response or request, which
    // might trigger some other requests in the application. We set the current
    // trace to this socket's trace even if the write was unsuccessful, since
    // this might trigger some events in the application that would make
    // requests, e.g. report error
    set_current_trace(trace_);

    if (ret == -1) {
        // error
        return;
    }

    // If we get to this point, it means that the connection is open,
    // and a buffer was handed to the kernel.
    VERIFY(console_log, ret > 0, "write invalid return value");

    // At this point ret is > 0, which means that the connection is open, so
    // SetConnection should succeed.
    if (!conn_init_) {
        SetConnection();
    }

    if (role_client() &&
        (state_ == SocketState::WILL_WRITE || state_ == SocketState::READ)) {
        ++num_requests_;
        RequestLogWrapper log;
        FillRequestLog(log);
        logger.Log(log.get());
    }

    state_ = SocketState::WROTE;
}

void SocketCallback::BeforeClose() {}

void SocketCallback::AfterClose(int ret) {}
