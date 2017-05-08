#include "socket_entry.h"

#include <assert.h>
#include <iostream>

Connid::Connid()
    : local_ip('.', INET6_ADDRSTRLEN), peer_ip('.', INET6_ADDRSTRLEN) {}

void Connid::print() const {
    std::cout << "(" << local_ip << "):" << local_port << " -> (" << peer_ip
              << "):" << peer_port << std::endl;
}

SocketEntry::SocketEntry(const int fd, const trace_id_t trace,
                         const SocketType type)
    : fd_(fd),
      trace_(trace),
      type_(type),
      state_(type == SocketType::ACCEPTED ? SocketState::READ
                                          : SocketState::WRITE),
      has_connid_(false) {
    http_parser_init(&parser_, HTTP_REQUEST);
    parser_.data = this;
}

bool SocketEntry::has_connid() { return has_connid_; }

unsigned short get_port(const struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ((struct sockaddr_in*)sa)->sin_port;
    }
    return ((struct sockaddr_in6*)sa)->sin6_port;
}

int SocketEntry::SetConnid() {
    struct sockaddr addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    int ret;
    const char* dst;

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
