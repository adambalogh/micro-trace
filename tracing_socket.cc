#include "tracing_socket.h"

#include <assert.h>
#include <iostream>

#include "posix_defs.h"

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

void TracingSocket::BeforeRead() {}

void TracingSocket::AfterRead(const void *buf, size_t len) {
    // Set connid if it hasn't been set before, e.g. in case of
    // when a socket was opened using connect().
    if (!has_connid()) {
        SetConnid();
    }

    set_current_trace(trace());
    DLOG("%d received %ld bytes", fd(), ret);

    state_ = SocketState::READ;
}

void TracingSocket::BeforeWrite() {
    if (!role_client()) return;

    if (state_ == SocketState::WILL_WRITE || state_ == SocketState::READ) {
        printf("writing start of new message\n");
    }
}

void TracingSocket::AfterWrite(ssize_t len) {
    // Set connid if it hasn't been set before, e.g. in case of
    // when a socket was opened using connect().
    if (!has_connid()) {
        SetConnid();
    }

    if (valid_trace(trace())) {
        set_current_trace(trace());

        // We are only interested in write to sockets that we opened to
        // other servers, aka where we act as the client
        if (role_client()) {
            LOG("sent %ld bytes", len);
        }
    }

    state_ = SocketState::WRITE;
}

ssize_t TracingSocket::RecvFrom(void *buf, size_t len, int flags,
                                struct sockaddr *src_addr, socklen_t *addrlen) {
    ssize_t ret = orig.orig_recvfrom(fd(), buf, len, flags, src_addr, addrlen);
    if (ret == -1) {
        return ret;
    }

    AfterRead(buf, ret);
    return ret;
}

ssize_t TracingSocket::Recv(void *buf, size_t len, int flags) {
    ssize_t ret = orig.orig_recv(fd(), buf, len, flags);
    if (ret == -1) {
        return ret;
    }
    AfterRead(buf, ret);
    return ret;
}

ssize_t TracingSocket::Read(void *buf, size_t count) {
    ssize_t ret = orig.orig_read(fd(), buf, count);
    if (ret == -1) {
        return ret;
    }
    AfterRead(buf, ret);
    return ret;
}

ssize_t TracingSocket::Send(const void *buf, size_t len, int flags) {
    BeforeWrite();
    ssize_t ret = orig.orig_send(fd(), buf, len, flags);
    AfterWrite(ret);
    return ret;
}

ssize_t TracingSocket::Write(const void *buf, size_t count) {
    BeforeWrite();
    ssize_t ret = orig.orig_write(fd(), buf, count);
    AfterWrite(ret);
    return ret;
}

ssize_t TracingSocket::Writev(const struct iovec *iov, int iovcnt) {
    BeforeWrite();
    ssize_t ret = orig.orig_writev(fd(), iov, iovcnt);
    AfterWrite(ret);
    return ret;
}

int TracingSocket::Close() {
    int ret = orig.orig_close(fd());
    return ret;
}
