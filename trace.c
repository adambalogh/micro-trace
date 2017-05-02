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

#include "trace.h"

void handle_recv(const int sockfd, const void* buf, const size_t ret) {
    int parent_fd = get_parent(sockfd);
    set_current_socket(parent_fd);
    LOG("%d received %ld bytes", sockfd, ret);
    /*fwrite(buf, MIN(ret, 40), 1, stdout);*/
    /*printf("\n");*/
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    orig_recv_t orig_recv = (orig_recv_t) orig("recv");
    ssize_t ret =  orig_recv(sockfd, buf, len, flags);
    if (ret == -1) {
        return ret;
    }

    handle_recv(sockfd, buf, ret);
    return ret;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    orig_recvfrom_t orig_recvfrom = (orig_recvfrom_t) orig("recvfrom");
    ssize_t ret =  orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (ret == -1) {
        return ret;
    }

    handle_recv(sockfd, buf, ret);
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
    /*fwrite(buf, MIN(ret, 40), 1, stdout);*/
    /*printf("\n");                        */
    return ret;
}

int socket(int domain, int type, int protocol) {
    orig_socket_t orig_socket = (orig_socket_t) orig("socket");
    int ret = orig_socket(domain, type, protocol);

    set_parent(ret, current_socket);
    LOG("new socket: %d", ret);
    return ret;
}

int close(int fd) {
    orig_close_t orig_close = (orig_close_t) orig("close");
    int ret = orig_close(fd);
    if (ret == 0) {
        del_parent(fd);
    }
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
