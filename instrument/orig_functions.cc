#include "orig_functions.h"

#include "common.h"

#define ORIG(func, name)                        \
    do {                                        \
        func = (decltype(func))find_orig(name); \
    } while (0)

namespace microtrace {

void *find_orig(const char *name) { return dlsym(RTLD_NEXT, name); }

OriginalFunctions &orig() {
    static OriginalFunctionsImpl o;
    return o;
}

OriginalFunctionsImpl::OriginalFunctionsImpl() {
    ORIG(orig_socket, "socket");
    ORIG(orig_close, "close");
    ORIG(orig_recvfrom, "recvfrom");
    ORIG(orig_accept, "accept");
    ORIG(orig_accept4, "accept4");
    ORIG(orig_recv, "recv");
    ORIG(orig_read, "read");
    ORIG(orig_write, "write");
    ORIG(orig_writev, "writev");
    ORIG(orig_send, "send");
    ORIG(orig_sendto, "sendto");
    ORIG(orig_sendmsg, "sendmsg");

    ORIG(orig_uv_accept, "uv_accept");
    ORIG(orig_uv_getaddrinfo, "uv_getaddrinfo");
}

int OriginalFunctionsImpl::socket(int domain, int type, int protocol) const {
    return orig_socket(domain, type, protocol);
}

int OriginalFunctionsImpl::close(int fd) const { return orig_close(fd); }

int OriginalFunctionsImpl::accept(int sockfd, struct sockaddr *addr,
                                  socklen_t *addrlen) const {
    return orig_accept(sockfd, addr, addrlen);
}

int OriginalFunctionsImpl::accept4(int sockfd, struct sockaddr *addr,
                                   socklen_t *addrlen, int flags) const {
    return orig_accept4(sockfd, addr, addrlen, flags);
}

ssize_t OriginalFunctionsImpl::recv(int sockfd, void *buf, size_t len,
                                    int flags) const {
    return orig_recv(sockfd, buf, len, flags);
}

ssize_t OriginalFunctionsImpl::read(int fd, void *buf, size_t count) const {
    return orig_read(fd, buf, count);
}

ssize_t OriginalFunctionsImpl::recvfrom(int sockfd, void *buf, size_t len,
                                        int flags, struct sockaddr *src_addr,
                                        socklen_t *addrlen) const {
    return orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

ssize_t OriginalFunctionsImpl::write(int fd, const void *buf,
                                     size_t count) const {
    return orig_write(fd, buf, count);
}

ssize_t OriginalFunctionsImpl::writev(int fd, const struct iovec *iov,
                                      int iovcnt) const {
    return orig_writev(fd, iov, iovcnt);
}

ssize_t OriginalFunctionsImpl::send(int sockfd, const void *buf, size_t len,
                                    int flags) const {
    return orig_send(sockfd, buf, len, flags);
}

ssize_t OriginalFunctionsImpl::sendto(int sockfd, const void *buf, size_t len,
                                      int flags,
                                      const struct sockaddr *dest_addr,
                                      socklen_t addrlen) const {
    return orig_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t OriginalFunctionsImpl::sendmsg(int sockfd, const struct msghdr *msg,
                                       int flags) const {
    return orig_sendmsg(sockfd, msg, flags);
}

int OriginalFunctionsImpl::sendmmsg(int sockfd, struct mmsghdr *msgvec,
                                    unsigned int vlen,
                                    unsigned int flags) const {
    return orig_sendmmsg(sockfd, msgvec, vlen, flags);
}

int OriginalFunctionsImpl::uv_accept(uv_stream_t *server,
                                     uv_stream_t *client) const {
    return orig_uv_accept(server, client);
}

int OriginalFunctionsImpl::uv_getaddrinfo(uv_loop_t *loop,
                                          uv_getaddrinfo_t *req,
                                          uv_getaddrinfo_cb getaddrinfo_cb,
                                          const char *node, const char *service,
                                          const struct addrinfo *hints) const {
    return orig_uv_getaddrinfo(loop, req, getaddrinfo_cb, node, service, hints);
}
}
