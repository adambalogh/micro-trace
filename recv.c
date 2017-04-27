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

void* orig(const char* name) {
    return dlsym(RTLD_NEXT, name);
}

ssize_t read(int fd, void *buf, size_t count) {
    printf("read: %d\n", fd);

    orig_read_t orig_read = (orig_read_t) orig("read");

    return orig_read(fd, buf, count);
}

int socket(int domain, int type, int protocol) {
    orig_socket_t orig_socket = (orig_socket_t) orig("socket");

    int ret = orig_socket(domain, type, protocol);
    printf("socket: %d\n", ret);

    return ret;
}

int epoll_wait(int epfd, struct epoll_event *events,
               int maxevents, int timeout) {
	orig_epoll_wait_t orig_epoll = (orig_epoll_wait_t) orig("epoll_wait");

	return orig_epoll(epfd, events, maxevents, timeout);
}
