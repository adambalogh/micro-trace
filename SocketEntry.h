#ifndef _SOCKET_ENTRY_H_
#define _SOCKET_ENTRY_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string>

#include "http_parser.h"
#include "uthash.h"

#include "helpers.h"


struct Connid {
 public:
    Connid();

    void print() const;

    std::string local_ip;
    unsigned short local_port;
    std::string peer_ip;
    unsigned short peer_port;
};

class SocketEntry {
 public:
    enum socket_type {
        SOCKET_OPENED,
        SOCKET_ACCEPTED
    };

    SocketEntry(const int fd, const trace_id_t trace,
            const socket_type socket);

    bool has_connid();
    void SetConnid();

    bool type_accepted() const {
        return type_ == SOCKET_ACCEPTED;
    }

    bool type_opened() const {
        return type_ == SOCKET_OPENED;
    }

    int fd() const { return fd_; }
    trace_id_t trace() const { return trace_; }

 private:
    int fd_;
    trace_id_t trace_;
    socket_type type_;
    http_parser parser_;

    bool has_connid_;
    Connid connid_;
};

#endif

