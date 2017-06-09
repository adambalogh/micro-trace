#pragma once

#include <string>

#include "request_log.pb.h"
#include "trace.h"

namespace microtrace {

typedef std::function<ssize_t()> IoFunction;
typedef std::function<int()> CloseFunction;

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

/*
 * InstrumentedSocket is an abstraction over an OS socket, which helps tracing
 * sockets by merging all socket operations into Write, Read, and Close.
 *
 * The actual socket operation should be passed as an argument to each of these
 * functions.
 */
class InstrumentedSocket {
   public:
    virtual ~InstrumentedSocket() = default;

    /*
     * Read should execute the given IO function, which is required to be a
     * close operation. In addition, it executes tracing logic.
     */
    virtual ssize_t Read(const void* buf, size_t len, IoFunction fun) = 0;

    /*
     * Write should execute the given IO function, which is required to be a
     * write operation. In addition, it executes tracing logic.
     */
    virtual ssize_t Write(const struct iovec* iov, int iovcnt,
                          IoFunction fun) = 0;

    /*
     * Close executes the given function which is required to close the
     * underlying socket. In addition, it executes tracing logic.
     */
    virtual int Close(CloseFunction fun) = 0;

    virtual int fd() const = 0;
    virtual SocketRole role() const = 0;
};

class AbstractInstrumentedSocket : public InstrumentedSocket {
   public:
    AbstractInstrumentedSocket(int sockfd, const trace_id_t trace,
                               const SocketRole role, const SocketState state);

    int fd() const override { return sockfd_; }
    SocketRole role() const override { return role_; }

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

    const int sockfd_;

    trace_id_t trace_;

    /*
     * Indicates whether this socket is being used as a client, or as a
     * server in this connection, e.g. whenever a socket is accepted, it will
     * act as a server. This is a valid assumption because we require
     * aplications to use a request-response communication method.
     */
    const SocketRole role_;

    /*
     * Records the state of the socket.
     */
    SocketState state_;

    /*
     * Number of transactions that went through this socket.
     */
    int num_transactions_;

    /*
     * Indicates if conn_ has been successfully set up, it DOES NOT indicate
     * whether the socket is connected or not, although if its value is
     * true, it also means that the socket is connected.
     */
    bool conn_init_;

    /*
     * The connection this socket represents. Remains the same throughout
     * the socket's lifetime, and becomes invalid after Close() has been called.
     */
    Connection conn_;
};
}
