#include "socket.h"


void connid_print(const connid_t *connid) {
    printf("(%s):%hu -> (%s):%hu\n", connid->local_ip, connid->local_port,
            connid->peer_ip, connid->peer_port);
}

socket_entry_t* socket_entry_new(const int fd, const trace_id_t trace,
        const socket_type type) {
    socket_entry_t* entry = malloc(sizeof(socket_entry_t));
    entry->fd = fd;
    entry->trace = trace;
    entry->type = type;

    entry->parser = malloc(sizeof(http_parser));
    http_parser_init(entry->parser, HTTP_REQUEST);
    entry->parser->data = entry;

    entry->connid_set = 0;
    entry->connid.local_ip = malloc(sizeof(char) * INET6_ADDRSTRLEN);
    entry->connid.peer_ip = malloc(sizeof(char) * INET6_ADDRSTRLEN);

    return entry;
}

void socket_entry_free(socket_entry_t* sock) {
    free(sock->parser);
    free(sock->connid->local_ip);
    free(sock->connid->peer_ip);
    free(sock);
}

int socket_entry_connid_set(socket_entry_t* sock) {
    return sock->connid_set == 1;
}

unsigned short get_port(const struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }
    return (((struct sockaddr_in6*)sa)->sin6_port);
}

void socket_entry_set_connid(socket_entry_t* sock) {
    struct sockaddr addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    int ret;
    const char* dst;

    ret = getsockname(sock->fd, &addr, &addr_len);
    if (ret != 0) {
        // error
    }
    sock->connid.local_port = get_port(&addr);
    dst = inet_ntop(addr.sa_family, &addr, sock->connid.local_ip, INET6_ADDRSTRLEN);
    if (dst == NULL) {
        // error
    }

    addr_len = sizeof(struct sockaddr);
    ret = getpeername(sock->fd, &addr, &addr_len);
    if (ret != 0) {
        // error
    }
    sock->connid.peer_port = get_port(&addr);
    dst = inet_ntop(addr.sa_family, &addr, sock->connid.peer_ip, INET6_ADDRSTRLEN);
    if (dst == NULL) {
        // error
    }

    sock->connid_set = 1;
}


