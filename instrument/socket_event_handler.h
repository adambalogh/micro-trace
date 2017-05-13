#pragma once

#include <string>

#include "common.h"

class TracingSocket;

struct Connection {
   public:
    Connection();

    std::string to_string() const;
    void print() const;

    std::string local_ip;
    unsigned short local_port;
    std::string peer_ip;
    unsigned short peer_port;
};

enum class SocketRole { CLIENT, SERVER };
enum class SocketState { WILL_READ, READ, WILL_WRITE, WRITE };

class SocketEventHandler {
   public:
    SocketEventHandler(TracingSocket& socket, const trace_id_t trace,
                       const SocketRole role)
        : socket_(socket),
          trace_(trace),
          role_(role),
          state_(role_ == SocketRole::SERVER ? SocketState::WILL_READ
                                             : SocketState::WILL_WRITE),
          num_requests_(0),
          conn_init_(false) {}

    ~SocketEventHandler() {}

    /*
         * Sets up the Connection ID of the socket.
         *
         * Should only be called if the socket is connected to an endpoint,
         * e.g. it has been connect()-ed or accept()-ed
         *
         * On success, it returns 0, otherwise it returns a standard error code,
         * returned by the Socket API functions.
         */
    int SetConnection();

    // Should be called before a read operation on the socket
    void BeforeRead();

    // Should be called after a *successful* read operation on the socket, e.g.
    // len > 0
    void AfterRead(const void* buf, size_t ret);

    // Should be called before a write operation on the socket
    void BeforeWrite();

    // Should be called after a *successful* write operation on the socket, e.g.
    // ret > 0
    void AfterWrite(const struct iovec* iov, int iovcnt, ssize_t ret);

    // Should be called before a socket is closed
    void BeforeClose();

    // Should be called after close has been called on the socket,
    // regardless of being successful or not.
    void AfterClose(int ret);

   protected:
    bool role_server() const { return role_ == SocketRole::SERVER; }
    bool role_client() const { return role_ == SocketRole::CLIENT; }

    trace_id_t trace() const { return trace_; }

   protected:
    TracingSocket& socket_;

    trace_id_t trace_;
    SocketRole role_;

    // Records the most recent operation executed on this socket
    SocketState state_;

    // Number of requests that went through this socket. If socket role is
    // client, this will indicate the number of requests sent, if the role
    // is server, it is the number of requests received.
    int num_requests_;

    // Indicates if conn_ has been successfully set up, it DOES NOT indicate
    // whether the socket is connected or not
    bool conn_init_;
    Connection conn_;
};
