#include "socket_handler.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <cstdlib>
#include <functional>
#include <string>

#include "common.h"
#include "orig_functions.h"

namespace microtrace {

Connection::Connection() {}

static ServerType GetServerType() {
    auto type = std::getenv("MICROTRACE_SERVER_TYPE");
    VERIFY(type != nullptr, "MICROTRACE_SERVER_TYPE is not defined");

    if (strcmp(type, "frontend") == 0) {
        return ServerType::FRONTEND;
    } else if (strcmp(type, "backend") == 0) {
        return ServerType::BACKEND;
    }
    VERIFY(false, "invalid MICROTRACE_SERVER_TYPE env {}", type);
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
      server_type_(GetServerType()),
      context_processed_(false),
      orig_(orig) {}

void SetConnectionEndPoint(
    const int fd, std::string* hostname,
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

    std::string ip;
    ip.resize(INET6_ADDRSTRLEN);
    dst = inet_ntop(tmp_sockaddr.ss_family, sockaddr_ptr, string_arr(ip),
                    ip.size());
    // inet_ntop should also be successful here
    VERIFY(dst == string_arr(ip), "inet_ntop was unsuccessful");

    // inet_ntop puts a null terminated string into ip
    ip.resize(strlen(string_arr(ip)));

    *hostname = ip + ":" + std::to_string(get_port(sockaddr_ptr));
}

int AbstractSocketHandler::SetConnection() {
    static char hostname_buf[400];
    VERIFY(gethostname(&hostname_buf[0], 400) == 0, "gethostname unsuccessful");
    const std::string hostname{&hostname_buf[0], strlen(&hostname_buf[0])};

    if (role_server()) {
        // Peer is the client
        SetConnectionEndPoint(
            sockfd_, &conn_.client_hostname,
            [](int fd, struct sockaddr* addr, socklen_t* addr_len) -> auto {
                return orig().getpeername(fd, addr, addr_len);
            });

        conn_.server_hostname = hostname;
    } else {
        // Peer is the server
        SetConnectionEndPoint(
            sockfd_, &conn_.server_hostname,
            [](int fd, struct sockaddr* addr, socklen_t* addr_len) -> auto {
                return orig().getpeername(fd, addr, addr_len);
            });

        conn_.client_hostname = hostname;
    }

    conn_init_ = true;
    return 0;
}
}
