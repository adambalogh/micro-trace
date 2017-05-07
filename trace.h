#ifndef _TRACE_H_
#define _TRACE_H_

#include <pthread.h>
#include <stdint.h>

#include "uv.h"
#include "http_parser.h"

#include "helpers.h"

#define LOG(msg, ...)                 \
    pthread_t thread =  pthread_self(); \
    printf("[t:%lu %d]: ", thread % 10000, current_trace); \
    printf(msg, __VA_ARGS__);         \
    printf("\n");

#ifdef DEBUG
#define DLOG(msg, ...) LOG(msg, __VA_ARGS__)
#else
#define DLOG(msg, ...)
#endif

#define UNDEFINED_TRACE -2

typedef int32_t trace_id_t;

__thread trace_id_t current_trace = UNDEFINED_TRACE;

/* Libc functions */

typedef void (*orig_free_t)(void *ptr);
typedef int (*orig_socket_t)(int domain, int type, int protocol);
typedef int (*orig_close_t)(int fd);
typedef ssize_t (*orig_recvfrom_t)(int sockfd, void *buf, size_t len, int flags,
                                   struct sockaddr *src_addr, socklen_t *addrlen);
typedef ssize_t (*orig_send_t)(int sockfd, const void *buf, size_t len, int flags);
typedef int (*orig_accept_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
typedef int (*orig_accept4_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
typedef ssize_t (*orig_recv_t)(int sockfd, void *buf, size_t len, int flags);
typedef ssize_t (*orig_read_t)(int fd, void *buf, size_t count);
typedef ssize_t (*orig_write_t)(int fd, const void *buf, size_t count);
typedef ssize_t (*orig_writev_t)(int fd, const struct iovec *iov, int iovcnt);

/* Libuv functions */

typedef int (*orig_uv_accept_t)(uv_stream_t* server, uv_stream_t* client);
typedef int (*orig_uv_getaddrinfo_t)(uv_loop_t* loop, uv_getaddrinfo_t* req,
        uv_getaddrinfo_cb getaddrinfo_cb, const char* node, const char* service,
        const struct addrinfo* hints);

typedef enum {
    SOCKET_ACCEPTED, // socket was initiated by client, and accepted by server
    SOCKET_OPENED // socket was opened by server
} socket_type;

/* 
 * Uniquely identifies a connection between to machines.
 */
typedef struct {
    const char* local_ip;
    unsigned short local_port;
    const char* peer_ip;
    unsigned short peer_port;
} conn_id_t;

/*
 * A socket_entry_t is assigned to every socket.
 *
 * Because of Connection: Keep-Alive, a socket may be reused for several
 * request-reply sequences, therefore a pair of sockets cannot uniquely
 * idenfity a trace.
 *
 * The assigned socket_entry must be removed if the underlying socket is closed.
 */
typedef struct {
    int fd;
    trace_id_t trace;
    socket_type type;
    OWNS(http_parser *parser);

    char addresses_set;
    struct sockaddr local_addr;
    size_t local_addr_len;
    struct sockaddr peer_addr;
    size_t peer_addr_len;

    UT_hash_handle hh; // name must be hh to work with macros
} socket_entry_t;

socket_entry_t* socket_entry_new(const int fd, const trace_id_t trace,
        const socket_type type) {
    socket_entry_t* entry = malloc(sizeof(socket_entry_t));
    entry->fd = fd;
    entry->trace = trace;
    entry->type = type;
    entry->parser = malloc(sizeof(http_parser));
    http_parser_init(entry->parser, HTTP_REQUEST);
    entry->parser->data = entry;
    entry->addresses_set = false;

    return entry;
}

void socket_entry_free(socket_entry_t* sock) {
    free(sock->parser);
    free(sock);
}

int socket_entry_is_addresses_set(socket_entry_t* sock) {
    return sock->addresses_set;
}

void socket_entry_set_addresses(socket_entry_t* sock) {
    int ret;
    ret = getaddrname(sock->fd, &sock->local_addr);
    if (ret != 0) {
    }
    ret = getpeername(sock->fd, &sock->peer_addr);
    if (ret != 0) {
    }
}

int socket_type_accepted(const socket_entry_t *sock) {
    return sock->type == SOCKET_ACCEPTED;
}

int socket_type_opened(const socket_entry_t *sock) {
    return sock->type == SOCKET_OPENED;
}

typedef struct {
    BORROWS(void *req_ptr);
    uv_getaddrinfo_cb orig_cb;
    trace_id_t id;

    UT_hash_handle hh; // name must be hh to work with macros
} trace_wrap_t;

// TODO make these thread-safe
socket_entry_t* sockets = NULL;
trace_wrap_t* trace_wraps = NULL;

void* orig(const char* name) {
    return dlsym(RTLD_NEXT, name);
}

int valid_trace(const trace_id_t trace) {
    return trace != UNDEFINED_TRACE && trace != -1;
}

void set_current_trace(const trace_id_t trace) {
    if (valid_trace(trace)) {
        current_trace = trace;
    }
}

void add_socket_entry(socket_entry_t* entry) {
    HASH_ADD_INT(sockets, fd, entry);
}

socket_entry_t* get_socket_entry(const int sockfd) {
    socket_entry_t* entry;
    HASH_FIND_INT(sockets, &sockfd, entry);
    return entry;
}

trace_id_t get_socket_trace(const int sockfd) {
    const socket_entry_t* entry = get_socket_entry(sockfd);
    if (entry == NULL) {
        return UNDEFINED_TRACE;
    }
    return entry->trace;
}

void del_socket_entry(const int sockfd) {
    socket_entry_t* entry = get_socket_entry(sockfd);
    if (entry != NULL) {
        HASH_DEL(sockets, entry);  
        free_socket_entry(entry);
    }
}

void add_trace_wrap(trace_wrap_t* trace) {
    HASH_ADD_PTR(trace_wraps, req_ptr, trace);
}

trace_wrap_t* get_trace_wrap(void* req_ptr) {
    trace_wrap_t* trace;
    HASH_FIND_PTR(trace_wraps, &req_ptr, trace);
    assert(trace != NULL);
    return trace;
}

void del_trace_wrap(trace_wrap_t* trace) {
    assert(trace != NULL);
    HASH_DEL(trace_wraps, trace);
    free(trace);
}

#endif
