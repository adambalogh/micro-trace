#include "socket_handler.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <functional>
#include <string>

#include "common.h"
#include "orig_functions.h"

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

AbstractSocketHandler::AbstractSocketHandler(int sockfd, const SocketRole role,
                                             const SocketState state,
                                             const OriginalFunctions& orig)
    : sockfd_(sockfd),
      context_(nullptr),
      role_(role),
      state_(state),
      num_transactions_(0),
      conn_init_(false),
      type_(SocketType::BLOCKING),
      orig_(orig) {
    DLOG("New AbstractSocketHandler %d", sockfd);
}

void SetConnectionEndPoint(
    const int fd, std::string* ip, short unsigned* port,
    std::function<int(int, struct sockaddr*, socklen_t*)> fn) {
    sockaddr_storage tmp_sockaddr;
    sockaddr* const sockaddr_ptr = reinterpret_cast<sockaddr*>(&tmp_sockaddr);
    socklen_t addr_len = sizeof(tmp_sockaddr);

    int ret;
    const char* dst;

    ret = fn(fd, sockaddr_ptr, &addr_len);
    // At this point, both getsockname and getpeername should
    // be successful
    VERIFY(ret == 0, "get(sock|peer)name was unsuccessful");

    *port = get_port(sockaddr_ptr);
    dst = inet_ntop(tmp_sockaddr.ss_family, sockaddr_ptr, string_arr(*ip),
                    ip->size());
    // inet_ntop should also be successful here
    VERIFY(dst == string_arr(*ip), "inet_ntop was unsuccessful");

    // inet_ntop puts a null terminated string into ip
    ip->resize(strlen(string_arr(*ip)));
}

int AbstractSocketHandler::SetConnection() {
    if (role_server()) {
        // Peer is the client
        SetConnectionEndPoint(
            sockfd_, &conn_.client_ip, &conn_.client_port,
            [](int fd, struct sockaddr* addr, socklen_t* addr_len) -> auto {
                return orig().getpeername(fd, addr, addr_len);
            });
        // We are the server
        SetConnectionEndPoint(
            sockfd_, &conn_.server_ip, &conn_.server_port,
            [](int fd, struct sockaddr* addr, socklen_t* addr_len) -> auto {
                return orig().getsockname(fd, addr, addr_len);
            });

    } else {
        // Peer is the server
        SetConnectionEndPoint(
            sockfd_, &conn_.server_ip, &conn_.server_port,
            [](int fd, struct sockaddr* addr, socklen_t* addr_len) -> auto {
                return orig().getpeername(fd, addr, addr_len);
            });
        // We are the client
        SetConnectionEndPoint(
            sockfd_, &conn_.client_ip, &conn_.client_port,
            [](int fd, struct sockaddr* addr, socklen_t* addr_len) -> auto {
                return orig().getsockname(fd, addr, addr_len);
            });
    }

    conn_init_ = true;
    return 0;
}
}
