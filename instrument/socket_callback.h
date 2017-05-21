#pragma once

#include <string>

#include "request_log.pb.h"
#include "trace.h"

struct RequestLogWrapper;

/*
 * Identifies a unique connection between two machines.
 *
 * Sockets can be divided into 2 groups: server and client role (see SocketRole
 * for explanation). Since both sockets in a connection know their role, the
 * connection objects should look the same on both endpoints. This makes it
 * easier to match logs.
 *
 * We use strings for storing IP addressed in order to make it easier to debug
 * logs.
 *
 * Note: connnections are not unique in time.
 */
struct Connection {
   public:
    Connection();

    std::string to_string() const;

    std::string client_ip;
    unsigned short client_port;
    std::string server_ip;
    unsigned short server_port;
};

/*
 * We require applications to use a request-response-based communication method
 * with a client-server model, which means that certain sockets will only
 * receive requests, these are Server sockets, and other sockets will only send
 * requests, these are Client sockets. As a result, we can divide sockets into
 * these 2 groups. We know that a socket is a Client if it was connect()-ed to
 * and endpoint, and a Server if it was accept()-ed. As an example, it wouldn't
 * work with WebSockets which supports server-sent events.
 *
 * In addition, if a socket is a Client, we know that its first operation will
 * be a write(), and if it is a Server, it will do a read() first.
 */
enum class SocketRole { CLIENT, SERVER };

enum class SocketState { WILL_READ, READ, WILL_WRITE, WROTE, CLOSED };

class SocketCallback {
   public:
    SocketCallback(int sockfd, const trace_id_t trace, const SocketRole role)
        : sockfd_(sockfd),
          trace_(trace),
          role_(role),
          state_(role_ == SocketRole::SERVER ? SocketState::WILL_READ
                                             : SocketState::WILL_WRITE),
          num_requests_(0),
          conn_init_(false) {}

    virtual ~SocketCallback() = default;

    // Should be called after the socket was accepted, if it was accepted.
    virtual void AfterAccept();

    // Should be called before a read operation on the socket
    virtual void BeforeRead();

    // Should be called after a *successful* read operation on the socket, e.g.
    // len > 0
    virtual void AfterRead(const void* buf, ssize_t ret);

    // Should be called before a write operation on the socket
    virtual void BeforeWrite();

    // Should be called after a *successful* write operation on the socket, e.g.
    // ret > 0
    virtual void AfterWrite(const struct iovec* iov, int iovcnt, ssize_t ret);

    // Should be called before a socket is closed
    virtual void BeforeClose();

    // Should be called after close has been called on the socket,
    // regardless of being successful or not.
    virtual void AfterClose(int ret);

    int fd() const { return sockfd_; }

   protected:
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

    bool role_server() const { return role_ == SocketRole::SERVER; }
    bool role_client() const { return role_ == SocketRole::CLIENT; }

    trace_id_t trace() const { return trace_; }

    void FillRequestLog(RequestLogWrapper& log);

   protected:
    const int sockfd_;

    /*
     * The trace currently associated with this socket. This might change
     * throughout the lifecycle of the socket, for example if applications
     * use connection pooling.
     */
    trace_id_t trace_;

    /*
     * Indicates whether this socket is being used as a client, or as a server
     * in this connection, e.g. whenever a socket is accepted, it will act as a
     * server. This is a valid assumption because we require aplications to use
     * a request-response communication method.
     */
    const SocketRole role_;

    /*
     * Records the state of the socket.
     */
    SocketState state_;

    /*
     * Number of requests that went through this socket. If socket role is
     * client, this will indicate the number of requests sent, if the role
     * is server, it is the number of requests received.
     */
    int num_requests_;

    /*
     * Indicates if conn_ has been successfully set up, it DOES NOT indicate
     * whether the socket is connected or not, although if its value is true,
     * it also means that the socket is connected.
     */
    bool conn_init_;

    /*
     * The connection this socket represents. Remains the same throughout the
     * socket's lifetime, and becomes invalid after Close() has been called.
     */
    Connection conn_;
};
