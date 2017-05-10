#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string>

#include "common.h"
#include "socket_interface.h"

/*
 * Uniquely identifies a connection between to machines.
 */
struct Connid {
   public:
    Connid();

    std::string to_string() const;
    void print() const;

    std::string local_ip;
    unsigned short local_port;
    std::string peer_ip;
    unsigned short peer_port;
};

enum class SocketRole { CLIENT, SERVER };
enum class SocketState { WILL_READ, READ, WILL_WRITE, WRITE };

/*
 * A TracingSocket is a wrapper around a regular socket.
 *
 * Because of Connection: Keep-Alive, a socket may be reused for several
 * request-reply sequences, therefore a pair of sockets cannot uniquely
 * idenfity a trace.
 *
 * The assigned socket_entry must be removed if the underlying socket
 * is closed.
 */
class TracingSocket : public SocketInterface {
   public:
    TracingSocket(const TracingSocket &) = delete;

    TracingSocket(const int fd, const trace_id_t trace, const SocketRole role);

    bool has_connid();

    /*
     * Sets up the Connection ID of the socket.
     *
     * Should only be called if the socket is connected to an endpoint,
     * e.g. it has been connect()-ed or accept()-ed
     *
     * On success, it returns 0, otherwise it returns a standard error code,
     * returned by the Socket API functions.
     */
    int SetConnid();

    ssize_t RecvFrom(void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen) override;
    ssize_t Recv(void *buf, size_t len, int flags) override;
    ssize_t Read(void *buf, size_t count) override;
    ssize_t Write(const void *buf, size_t count) override;
    ssize_t Writev(const struct iovec *iov, int iovcnt) override;
    ssize_t Send(const void *buf, size_t len, int flags) override;
    ssize_t SendTo(int sockfd, const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr,
                   socklen_t addrlen) override;
    int Close() override;

    bool role_server() const { return role_ == SocketRole::SERVER; }
    bool role_client() const { return role_ == SocketRole::CLIENT; }

    int fd() const override { return fd_; }
    trace_id_t trace() const { return trace_; }

   private:
    // Should be called before a read operation on the socket
    void BeforeRead();

    // Should be called after a *successful* read operation on the socket, e.g.
    // len > 0
    void AfterRead(const void *buf, size_t len);

    // Should be called before a write operation on the socket
    void BeforeWrite();

    // Should be called after a *successful* write operation on the socket, e.g.
    // ret > 0
    void AfterWrite(ssize_t ret);

    // File descriptor
    int fd_;

    trace_id_t trace_;
    SocketRole role_;

    // Records the most recent operation executed on this socket
    SocketState state_;

    int num_requests_;

    bool has_connid_;
    Connid connid_;
};
