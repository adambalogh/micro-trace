#ifndef _ORIG_TYPES_H_
#define _ORIG_TYPES_H_

#define FIND_ORIG(func, name)                 \
    do {                                      \
        if (func == NULL)                     \
            func = (decltype(func)) orig(name); \
    } while(0)


void* orig(const char* name) {
    return dlsym(RTLD_NEXT, name);
}


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

static __thread orig_socket_t orig_socket;
static __thread orig_close_t orig_close;
static __thread orig_recvfrom_t orig_recvfrom;
static __thread orig_send_t orig_send;
static __thread orig_accept_t orig_accept;
static __thread orig_accept4_t orig_accept4;
static __thread orig_recv_t orig_recv;
static __thread orig_read_t orig_read;
static __thread orig_write_t orig_write;
static __thread orig_writev_t orig_writev;

/* Libuv functions */

typedef int (*orig_uv_accept_t)(uv_stream_t* server, uv_stream_t* client);
typedef int (*orig_uv_getaddrinfo_t)(uv_loop_t* loop, uv_getaddrinfo_t* req,
        uv_getaddrinfo_cb getaddrinfo_cb, const char* node, const char* service,
        const struct addrinfo* hints);

static __thread orig_uv_accept_t orig_uv_accept;
static __thread orig_uv_getaddrinfo_t orig_uv_getaddrinfo;

#endif

