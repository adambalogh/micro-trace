#pragma once

#include <string>

#include "common.h"
#include "context.h"
#include "request_log.pb.h"

namespace microtrace {

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
 * SocketHandler is used for wrapping socket systemcalls.
 * Every Before* handler must be called before the corresponding socket
 * operation is executed, after which the appropriate After* handler must be
 * called.
 *
 * The tracing logic is implemented in SocketHandlers.
 */
class SocketHandler {
   public:
    /*
     * Indicates the result of calling a Before handler.
     *
     * Ok indicates that the operation should be executed, and Stop means
     * tha the operation should not continue, and the corresponding After
     * handler should not be called either.
     */
    enum class Result { Ok, Stop };

    virtual ~SocketHandler() = default;

    virtual void Async() = 0;

    virtual Result BeforeRead(const void* buf, size_t len) = 0;
    virtual void AfterRead(const void* buf, size_t len, ssize_t ret) = 0;

    virtual Result BeforeWrite(const struct iovec* iov, int iovcnt) = 0;
    virtual void AfterWrite(const struct iovec* iov, int iovcnt,
                            ssize_t ret) = 0;

    virtual Result BeforeClose() = 0;
    virtual void AfterClose(int ret) = 0;

    virtual int fd() const = 0;
    virtual SocketRole role() const = 0;
};

class AbstractSocketHandler : public SocketHandler {
   public:
    AbstractSocketHandler(int sockfd, const SocketRole role,
                          const SocketState state);

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

    bool has_context() const { return static_cast<bool>(context_); }
    const Context& context() const {
        VERIFY(has_context(), "context() called when it is empty");
        return *context_;
    }

    const int sockfd_;

    /*
     * Stores the current context. Initially empty.
     */
    std::unique_ptr<Context> context_;

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