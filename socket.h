#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>

#include "http_parser.h"
#include "uthash.h"

#include "helpers.h"


/* 
 * Uniquely identifies a connection between to machines.
 */
typedef struct {
    OWNS(char* local_ip);
    unsigned short local_port;
    OWNS(char* peer_ip);
    unsigned short peer_port;
} connid_t;

void connid_print(const connid_t *connid);

typedef enum {
    SOCKET_ACCEPTED, // socket was initiated by client, and accepted by server
    SOCKET_OPENED // socket was opened by server
} socket_type;

/*
 * A socket_entry_t is assigned to every socket.
 *
 * Because of Connection: Keep-Alive, a socket may be reused for several
 * request-reply sequences, therefore a pair of sockets cannot uniquely
 * idenfity a trace.
 *
 * The assigned socket_entry must be removed if the underlying socket is closed.
 */
typedef struct {
    int fd;
    trace_id_t trace;
    socket_type type;
    OWNS(http_parser *parser);

    char connid_set;
    connid_t connid;

    UT_hash_handle hh; // name must be hh to work with macros
} socket_entry_t;

socket_entry_t* socket_entry_new(const int fd, const trace_id_t trace,
        const socket_type type);
void socket_entry_free(socket_entry_t* sock);

int socket_entry_connid_set(socket_entry_t* sock);

/*
 * Should only be called if the socket is connected to an endpoint,
 * e.g. it has been connect()-ed or accept()-ed
 */
void socket_entry_set_connid(socket_entry_t* sock);

static inline int socket_type_accepted(const socket_entry_t *sock) {
    return sock->type == SOCKET_ACCEPTED;
}

static inline int socket_type_opened(const socket_entry_t *sock) {
    return sock->type == SOCKET_OPENED;
}


#endif
