#include "socket_event_handler.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include <string>

#include "tracing_socket.h"

Connection::Connection()
    : local_ip('.', INET6_ADDRSTRLEN), peer_ip('.', INET6_ADDRSTRLEN) {}

std::string Connection::to_string() const {
    std::string str;
    str += "[";
    str += "local: " + local_ip + ":" + std::to_string(local_port);
    str += ", ";
    str += "peer: " + peer_ip + ":" + std::to_string(peer_port);
    str += "]";
    return str;
}

void Connection::print() const { std::cout << to_string() << std::endl; }

unsigned short get_port(const struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ((struct sockaddr_in*)sa)->sin_port;
    }
    return ((struct sockaddr_in6*)sa)->sin6_port;
}

int SocketEventHandler::SetConnection() {
    struct sockaddr addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    int ret;
    const char* dst;

    ret = getsockname(socket_.fd(), &addr, &addr_len);
    if (ret != 0) {
        return ret;
    }
    conn_.local_port = get_port(&addr);
    dst = inet_ntop(addr.sa_family, &addr, string_arr(conn_.local_ip),
                    conn_.local_ip.size());
    if (dst == NULL) {
        return errno;
    }
    // inet_ntop puts a null terminated string into local_ip
    conn_.local_ip.resize(strlen(string_arr(conn_.local_ip)));

    addr_len = sizeof(struct sockaddr);

    ret = getpeername(socket_.fd(), &addr, &addr_len);
    if (ret != 0) {
        return ret;
    }
    conn_.peer_port = get_port(&addr);
    dst = inet_ntop(addr.sa_family, &addr, string_arr(conn_.peer_ip),
                    conn_.peer_ip.size());
    if (dst == NULL) {
        return errno;
    }
    // inet_ntop puts a null terminated string into peer_ip
    conn_.peer_ip.resize(strlen(string_arr(conn_.peer_ip)));

    conn_init_ = true;
    return 0;
}

void SocketEventHandler::BeforeRead() {}

void SocketEventHandler::AfterRead(const void* buf, size_t ret) {
    assert(ret >= 0);

    // Set connid if it hasn't been set before, e.g. in case of
    // when a socket was opened using connect().
    if (!conn_init_) {
        SetConnection();
    }

    set_current_trace(trace_);

    if (ret > 0) {
        if (role_server() && (state_ == SocketState::WILL_READ ||
                              state_ == SocketState::WRITE)) {
            ++num_requests_;
        }

        DLOG("%d received %ld bytes", socket_.fd(), ret);
        state_ = SocketState::READ;
    }
}

void SocketEventHandler::BeforeWrite() {}

void SocketEventHandler::AfterWrite(const struct iovec* iov, int iovcnt,
                                    ssize_t ret) {
    assert(ret >= 0);

    // Set connid if it hasn't been set before, e.g. in case of
    // when a socket was opened using connect().
    if (!conn_init_) {
        SetConnection();
    }

    set_current_trace(trace_);

    if (ret > 0) {
        if (role_client() && (state_ == SocketState::WILL_WRITE ||
                              state_ == SocketState::READ)) {
            ++num_requests_;
        }

        DLOG("%d wrote %ld bytes", socket_.fd(), ret);
        state_ = SocketState::WRITE;
    }
}

void SocketEventHandler::BeforeClose() {}

void SocketEventHandler::AfterClose(int ret) {}
