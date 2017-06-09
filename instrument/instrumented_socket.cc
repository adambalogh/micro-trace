#include "instrumented_socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <string>

#include "common.h"

namespace microtrace {

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

static unsigned short get_port(const struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ntohs(((sockaddr_in*)sa)->sin_port);
    } else {
        return ntohs(((sockaddr_in6*)sa)->sin6_port);
    }
}

AbstractInstrumentedSocket::AbstractInstrumentedSocket(int sockfd,
                                                       const trace_id_t trace,
                                                       const SocketRole role,
                                                       const SocketState state)
    : sockfd_(sockfd),
      trace_(trace),
      role_(role),
      state_(state),
      num_transactions_(0),
      conn_init_(false) {}

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

int AbstractInstrumentedSocket::SetConnection() {
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
}
