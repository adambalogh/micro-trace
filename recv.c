#define _GNU_SOURCE

#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include "uthash.h"

#define UNDEFINED_SOCKET -2

#define LOG(msg, ...)                \
    printf("[%d]: ", current_socket); \
    printf(msg, __VA_ARGS__);        \
    printf("\n");


typedef int (*orig_epoll_wait_t)(int epfd, struct epoll_event *events,
                                 int maxevents, int timeout);
typedef int (*orig_socket_t)(int domain, int type, int protocol);
typedef ssize_t (*orig_recvfrom_t)(int sockfd, void *buf, size_t len, int flags,
                                   struct sockaddr *src_addr, socklen_t *addrlen);
typedef ssize_t (*orig_send_t)(int sockfd, const void *buf, size_t len, int flags);
typedef int (*orig_accept_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
typedef int (*orig_accept4_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
typedef ssize_t (*orig_recv_t)(int sockfd, void *buf, size_t len, int flags);

typedef struct {
    int fd;
    int parent_fd;

    UT_hash_handle hh;  // makes this struct hashable
} socket_entry_t;


int current_socket = -1;
socket_entry_t* sockets = NULL;


void* orig(const char* name) {
    return dlsym(RTLD_NEXT, name);
}

void set_current_socket(const int socket) {
    if (socket != UNDEFINED_SOCKET) {
        current_socket = socket;
    }
}

void set_parent(const int sockfd, const int parent_sockfd) {
    socket_entry_t* entry = (socket_entry_t*) malloc(sizeof(socket_entry_t));
    entry->fd = sockfd;
    entry->parent_fd = parent_sockfd;
    HASH_ADD_INT(sockets, fd, entry);
}

int get_parent(const int sockfd) {
    const socket_entry_t* entry;
    HASH_FIND_INT(sockets, &sockfd, entry);
    if (entry == NULL) {
        return UNDEFINED_SOCKET;
    }
    return entry->parent_fd;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    orig_recv_t orig_recv = (orig_recv_t) orig("recv");
    ssize_t ret =  orig_recv(sockfd, buf, len, flags);
    if (ret == -1) {
        return ret;
    }

    int parent_fd = get_parent(sockfd);
    set_current_socket(parent_fd);
    LOG("%d received %ld bytes", sockfd, ret);
    return ret;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    orig_recvfrom_t orig_recvfrom = (orig_recvfrom_t) orig("recvfrom");
    ssize_t ret =  orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (ret == -1) {
        return ret;
    }

    int parent_fd = get_parent(sockfd);
    set_current_socket(parent_fd);
    LOG("%d received %ld bytes", sockfd, ret);
    return ret;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    orig_accept_t orig_accept = (orig_accept_t) orig("accept");
    int ret = orig_accept(sockfd, addr, addrlen);
    time_t t = time(NULL);
    printf("@%ld: %d accepted socket: %d\n", t, sockfd, ret);
    if (ret != -1) {
        set_parent(ret, ret);
        current_socket = ret;
    }

    return ret;
}

int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    orig_accept4_t orig_accept = (orig_accept4_t) orig("accept4");
    int ret = orig_accept(sockfd, addr, addrlen, flags);
    time_t t = time(NULL);
    printf("@%ld: %d accepted socket: %d\n", t, sockfd, ret);
    if (ret != -1) {
        set_parent(ret, ret);
        current_socket = ret;
    }

    return ret;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    orig_send_t orig_send = (orig_send_t) orig("send");
    ssize_t ret = orig_send(sockfd, buf, len, flags);
    LOG("%d sent %ld bytes", sockfd, ret);
    return ret;
}

int socket(int domain, int type, int protocol) {
    orig_socket_t orig_socket = (orig_socket_t) orig("socket");
    int ret = orig_socket(domain, type, protocol);

    set_parent(ret, current_socket);
    LOG("new socket: %d", ret);
    return ret;
}

/*int epoll_wait(int epfd, struct epoll_event *events,                      */
/*               int maxevents, int timeout) {                              */
/*    orig_epoll_wait_t orig_epoll = (orig_epoll_wait_t) orig("epoll_wait");*/
/*    int ret = orig_epoll(epfd, events, maxevents, timeout);               */
/*    if (ret != 0) {                                                       */
/*        printf("epoll_wait: %d ready\n", ret);                            */
/*    }                                                                     */
/*    return ret;                                                           */
/*}                                                                         */
