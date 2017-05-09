#include "tracing_socket.h"

#include <assert.h>
#include <iostream>

Connid::Connid()
    : local_ip('.', INET6_ADDRSTRLEN), peer_ip('.', INET6_ADDRSTRLEN) {}

void Connid::print() const {
    std::cout << "(" << local_ip << "):" << local_port << " -> (" << peer_ip
              << "):" << peer_port << std::endl;
}

TracingSocket::TracingSocket(const int fd, const trace_id_t trace,
                             const SocketRole role)
    : fd_(fd),
      trace_(trace),
      role_(role),
      state_(role_ == SocketRole::SERVER ? SocketState::WILL_READ
                                         : SocketState::WILL_WRITE),
      has_connid_(false) {
    http_parser_init(&parser_, HTTP_REQUEST);
    parser_.data = this;
}

bool TracingSocket::has_connid() { return has_connid_; }

unsigned short get_port(const struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return ((struct sockaddr_in *)sa)->sin_port;
    }
    return ((struct sockaddr_in6 *)sa)->sin6_port;
}

int TracingSocket::SetConnid() {
    struct sockaddr addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    int ret;
    const char *dst;

    ret = getsockname(fd_, &addr, &addr_len);
    if (ret != 0) {
        return ret;
    }
    connid_.local_port = get_port(&addr);
    dst = inet_ntop(addr.sa_family, &addr, string_arr(connid_.local_ip),
                    connid_.local_ip.size());
    if (dst == NULL) {
        return errno;
    }

    addr_len = sizeof(struct sockaddr);
    ret = getpeername(fd_, &addr, &addr_len);
    if (ret != 0) {
        return ret;
    }
    connid_.peer_port = get_port(&addr);
    dst = inet_ntop(addr.sa_family, &addr, string_arr(connid_.peer_ip),
                    connid_.local_ip.size());
    if (dst == NULL) {
        return errno;
    }

    has_connid_ = true;
    return 0;
}

ssize_t TracingSocket::RecvFrom(int sockfd, void *buf, size_t len, int flags,
                                struct sockaddr *src_addr, socklen_t *addrlen) {
}
ssize_t TracingSocket::Send(int sockfd, const void *buf, size_t len,
                            int flags) {}
ssize_t TracingSocket::Recv(int sockfd, void *buf, size_t len, int flags) {}
ssize_t TracingSocket::Read(int fd, void *buf, size_t count) {}
ssize_t TracingSocket::Write(int fd, const void *buf, size_t count) {}
ssize_t TracingSocket::Writev(int fd, const struct iovec *iov, int iovcnt) {}
int TracingSocket::Close() {}
