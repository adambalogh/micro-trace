#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "uthash.h"

typedef int (*orig_epoll_wait_t)(int epfd, struct epoll_event *events,
                                 int maxevents, int timeout);
typedef int (*orig_socket_t)(int domain, int type, int protocol);
typedef ssize_t (*orig_read_t)(int fd, void *buf, size_t count);
typedef ssize_t (*orig_recvfrom_t)(int sockfd, void *buf, size_t len, int flags,
                                   struct sockaddr *src_addr, socklen_t *addrlen);
typedef ssize_t (*orig_send_t)(int sockfd, const void *buf, size_t len, int flags);
typedef int (*orig_accept_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

typedef struct {
    int fd;
    int parent_fd;
    UT_hash_handle hh;         /* makes this structure hashable */
} socket_entry_t;

__thread int current_socket = -1;
__thread socket_entry_t* sockets = NULL;

void* orig(const char* name) {
    return dlsym(RTLD_NEXT, name);
}

void add_parent(const int sockfd, const int parent_sockfd) {
    socket_entry_t* entry = (socket_entry_t*) malloc(sizeof(socket_entry_t));
    entry->fd = sockfd;
    entry->parent_fd = parent_sockfd;
    HASH_ADD_INT(sockets, fd, entry);  /* id: name of key field */
}

int get_parent(const int sockfd) {
    const socket_entry_t* entry;
    HASH_FIND_INT(sockets, &sockfd, entry);  /* s: output pointer */
    return entry->parent_fd;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    orig_accept_t orig_accept = (orig_accept_t) orig("accept");
    int ret = orig_accept(sockfd, addr, addrlen);
    if (ret != -1) {
        printf("got new connection: %d\n", ret);
        add_parent(ret, ret);
        if (current_socket == -1) {
            current_socket = ret;
        }
    }
    return ret;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    printf("%d: send %d\n", get_parent(sockfd), sockfd);
    orig_send_t orig_send = (orig_send_t) orig("send");
    return orig_send(sockfd, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    printf("%d: recvfrom %d\n", get_parent(sockfd), sockfd);
    orig_recvfrom_t orig_recvfrom = (orig_recvfrom_t) orig("recvfrom");
    return orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

int socket(int domain, int type, int protocol) {
    orig_socket_t orig_socket = (orig_socket_t) orig("socket");
    int ret = orig_socket(domain, type, protocol);

    add_parent(ret, current_socket);
    printf("%d: socket: %d\n", current_socket, ret);
    
    return ret;
}

int epoll_wait(int epfd, struct epoll_event *events,
               int maxevents, int timeout) {
	orig_epoll_wait_t orig_epoll = (orig_epoll_wait_t) orig("epoll_wait");
	return orig_epoll(epfd, events, maxevents, timeout);
}
