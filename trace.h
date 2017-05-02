#ifndef _TRACE_H_
#define _TRACE_H_

#define UNDEFINED_SOCKET -2

#define MIN(a, b) ((a)<(b)) ? (a) : (b)

#define LOG(msg, ...)                 \
    printf("[%d]: ", current_socket); \
    printf(msg, __VA_ARGS__);         \
    printf("\n");


typedef int (*orig_epoll_wait_t)(int epfd, struct epoll_event *events,
                                 int maxevents, int timeout);
typedef int (*orig_socket_t)(int domain, int type, int protocol);
typedef int (*orig_close_t)(int fd);
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

void del_parent(const int sockfd) {
    socket_entry_t* entry;
    HASH_FIND_INT(sockets, &sockfd, entry);  
    if (entry != NULL) {
        HASH_DEL(sockets, entry);  
    }
}

#endif
