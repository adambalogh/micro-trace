#ifndef _TRACE_H_
#define _TRACE_H_

#include <pthread.h>

#include "uv.h"

#define UNDEFINED_TRACE -2

// IMPORTANT evaluating expressions a and b multiple times shouldn't
// have any side-effect
#define MIN(a, b) ((a)<(b)) ? (a) : (b)

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

typedef int trace_id_t;

/* Libc functions */

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

typedef struct {
    int fd;
    trace_id_t trace;

    UT_hash_handle hh; 
} socket_entry_t;

typedef struct {
    void* req_ptr;
    uv_getaddrinfo_cb orig_cb;
    trace_id_t id;

    UT_hash_handle hh; 
} trace_wrap_t;

__thread trace_id_t current_trace = -1;

socket_entry_t* sockets = NULL;
trace_wrap_t* trace_wraps = NULL;

void* orig(const char* name) {
    return dlsym(RTLD_NEXT, name);
}

void set_current_trace(const trace_id_t trace) {
    if (trace != UNDEFINED_TRACE) {
        current_trace = trace;
    }
}

void set_trace(const int sockfd, const trace_id_t trace) {
    socket_entry_t* entry = (socket_entry_t*) malloc(sizeof(socket_entry_t));
    entry->fd = sockfd;
    entry->trace = trace;
    HASH_ADD_INT(sockets, fd, entry);
}

trace_id_t get_trace(const int sockfd) {
    const socket_entry_t* entry;
    HASH_FIND_INT(sockets, &sockfd, entry);
    if (entry == NULL) {
        return UNDEFINED_TRACE;
    }
    return entry->trace;
}

void del_socket_trace(const int sockfd) {
    socket_entry_t* entry;
    HASH_FIND_INT(sockets, &sockfd, entry);  
    if (entry != NULL) {
        HASH_DEL(sockets, entry);  
    }
}

trace_wrap_t* get_trace_wrap(void* req_ptr) {
    trace_wrap_t* trace;
    HASH_FIND_PTR(trace_wraps, &req_ptr, trace);
    return trace;
}

// TODO remove trace when req_ptr is freed
//
void add_trace_wrap(trace_wrap_t* trace) {
    HASH_ADD_PTR(trace_wraps, req_ptr, trace);
}

#endif
