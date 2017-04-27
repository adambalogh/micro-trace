#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef int (*orig_epoll_wait_t)(int epfd, struct epoll_event *events,
                                 int maxevents, int timeout);
typedef int (*orig_socket_t)(int domain, int type, int protocol);
typedef ssize_t (*orig_read_t)(int fd, void *buf, size_t count);
typedef ssize_t (*orig_recvfrom_t)(int sockfd, void *buf, size_t len, int flags,
                                   struct sockaddr *src_addr, socklen_t *addrlen);
typedef ssize_t (*orig_send_t)(int sockfd, const void *buf, size_t len, int flags);
typedef int (*orig_accept_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);


__thread int initiator_socket = -1;

void* orig(const char* name) {
    return dlsym(RTLD_NEXT, name);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    orig_accept_t orig_accept = (orig_accept_t) orig("accept");
    int ret = orig_accept(sockfd, addr, addrlen);
    if (ret != -1) {
        printf("got new connection: %d\n", ret);
        if (initiator_socket == -1) {
            initiator_socket = ret;
        }
    }
    return ret;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    printf("%d: send %d\n", initiator_socket, sockfd);
    orig_send_t orig_send = (orig_send_t) orig("send");
    return orig_send(sockfd, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    printf("%d: recvfrom %d\n", initiator_socket, sockfd);
    orig_recvfrom_t orig_recvfrom = (orig_recvfrom_t) orig("recvfrom");
    return orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

int socket(int domain, int type, int protocol) {
    orig_socket_t orig_socket = (orig_socket_t) orig("socket");
    int ret = orig_socket(domain, type, protocol);
    printf("%d: socket: %d\n", initiator_socket, ret);
    return ret;
}

int epoll_wait(int epfd, struct epoll_event *events,
               int maxevents, int timeout) {
	orig_epoll_wait_t orig_epoll = (orig_epoll_wait_t) orig("epoll_wait");
	return orig_epoll(epfd, events, maxevents, timeout);
}
