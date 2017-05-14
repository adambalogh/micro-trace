#include "socket_callback.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include <string>

#include "logger.h"

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

void Connection::print() const { std::cout << to_string() << std::endl; }

void SocketCallback::AfterAccept() { assert(role_ == SocketRole::SERVER); }

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
    socklen_t addr_len = sizeof(tmp_sockaddr);

    int ret;
    const char* dst;

    ret = fn(fd, (sockaddr*)&tmp_sockaddr, &addr_len);
    // At this point, both getsockname and getpeername should
    // be successful
    assert(ret == 0);

    *port = get_port((sockaddr*)&tmp_sockaddr);
    dst = inet_ntop(tmp_sockaddr.ss_family, (sockaddr*)&tmp_sockaddr,
                    string_arr(*ip), ip->size());
    // inet_ntop should also be successful here
    assert(dst == string_arr(*ip));

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

void SocketCallback::BeforeRead() { assert(state_ != SocketState::WILL_WRITE); }

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

    assert(ret > 0);

    // Set connid if it hasn't been set before, e.g. in case of when a socket
    // was opened using connect(). At this point ret is > 0, which means that
    // the connection is open, so SetConnection should succeed.
    if (!conn_init_) {
        SetConnection();
    }

    if (role_server() &&
        (state_ == SocketState::WILL_READ || state_ == SocketState::WROTE)) {
        ++num_requests_;
    }

    DLOG("%d received %ld bytes", sockfd_, ret);
    state_ = SocketState::READ;
}

void SocketCallback::BeforeWrite() { assert(state_ != SocketState::WILL_READ); }

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

    assert(ret > 0);

    // Set connid if it hasn't been set before, e.g. in case of when a socket
    // was opened using connect(). At this point ret is > 0, which means that
    // the connection is open, so SetConnection should succeed.
    if (!conn_init_) {
        SetConnection();
    }

    if (role_client() &&
        (state_ == SocketState::WILL_WRITE || state_ == SocketState::READ)) {
        ++num_requests_;
    }

    DLOG("%d wrote %ld bytes", sockfd_, ret);
    state_ = SocketState::WROTE;
}

void SocketCallback::BeforeClose() {}

void SocketCallback::AfterClose(int ret) {}
