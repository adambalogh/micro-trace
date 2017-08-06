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

AbstractSocketHandler::AbstractSocketHandler(int sockfd,
                                             const SocketState state,
                                             const OriginalFunctions& orig)
    : sockfd_(sockfd),
      context_(nullptr),
      state_(state),
      num_transactions_(0),
      type_(SocketType::BLOCKING),
      server_type_(GetServerType()),
      context_processed_(false),
      orig_(orig) {}
}
