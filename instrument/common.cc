#include "common.h"

#include <netinet/in.h>

#include "spdlog/spdlog.h"

namespace microtrace {

std::shared_ptr<spdlog::logger> console_log =
    spdlog::stdout_logger_mt("tracing_console_log");

unsigned short get_port(const struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ntohs(((sockaddr_in*)sa)->sin_port);
    } else if (sa->sa_family == AF_INET6) {
        return ntohs(((sockaddr_in6*)sa)->sin6_port);
    } else {
        VERIFY(false, "invalid socket address family");
    }
}

bool is_localhost(const struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ((sockaddr_in*)sa)->sin_addr.s_addr == INADDR_LOOPBACK;
    } else if (sa->sa_family == AF_INET6) {
        return false;
    } else {
        VERIFY(false, "invalid socket address family");
    }
}
}
