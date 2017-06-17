#include "common.h"

#include <netinet/in.h>

#include "spdlog/spdlog.h"

namespace microtrace {

std::shared_ptr<spdlog::logger> console_log =
    spdlog::stdout_logger_mt("tracing_console_log");

unsigned short get_port(const struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return ntohs(((sockaddr_in*)sa)->sin_port);
    } else {
        return ntohs(((sockaddr_in6*)sa)->sin6_port);
    }
}
}
