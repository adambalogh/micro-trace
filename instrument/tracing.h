#pragma once

/*
 * These are the functions that we instrument in order to trace requests.
 *
 * These functions will be preloaded for the dynamic linker using LD_PRELOAD.
 */
extern "C" {

int socket(int domain, int type, int protocol);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int close(int fd);

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);

ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t read(int fd, void *buf, size_t count);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);

ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags);

int uv_tcp_connect(uv_connect_t *req, uv_tcp_t *handle,
                   const struct sockaddr *addr, uv_connect_cb cb);
int uv_listen(uv_stream_t *stream, int backlog, uv_connection_cb cb);
int uv_accept(uv_stream_t *server, uv_stream_t *client);
int uv_getaddrinfo(uv_loop_t *loop, uv_getaddrinfo_t *req,
                   uv_getaddrinfo_cb getaddrinfo_cb, const char *node,
                   const char *service, const struct addrinfo *hints);
}
