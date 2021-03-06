#pragma once

#include <chrono>
#include <string>

#include "common.h"
#include "context.h"
#include "request_log.pb.h"

namespace microtrace {

struct OriginalFunctions;

/*
 * Identifies a unique connection between two machines.
 *
 * We use strings for storing IP addressed in order to make it easier to debug
 * logs.
 *
 * Note: connnections are not unique in time.
 */
struct Connection {
   public:
    Connection();

    std::string client_hostname;
    std::string server_hostname;
};

/*
 * A transaction is a request-respone sequence between this client and
 * a server.
 */
class Transaction {
   public:
    void Start() {
        system_start_ = std::chrono::system_clock::now();
        start_ = std::chrono::steady_clock::now();
    }

    void End() { end_ = std::chrono::steady_clock::now(); }

    time_t start() const {
        return std::chrono::system_clock::to_time_t(system_start_);
    }

    double duration() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_ -
                                                                     start_)
            .count();
    }

   private:
    std::chrono::time_point<std::chrono::system_clock> system_start_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
    std::chrono::time_point<std::chrono::steady_clock> end_;
};

/*
 * Wraps a proto::RequestLog. On destruction, it releases the fields that have
 * been borrowed, and not owned by the underlying RequestLog.
 */
struct RequestLogWrapper {
    ~RequestLogWrapper() {
        proto::Connection* conn = log.mutable_conn();
        conn->release_server_hostname();
        conn->release_client_hostname();
    }

    proto::RequestLog* operator->() { return &log; }
    const proto::RequestLog& get() const { return log; }

    proto::RequestLog log;
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

enum class SocketState { WILL_READ, READ, WILL_WRITE, WROTE, CLOSED };

enum class SocketOperation { WRITE, READ };

/*
 * Logical actions a socket can take.
 */
enum class SocketAction {
    SEND_REQUEST,
    RECV_REQUEST,
    SEND_RESPONSE,
    RECV_RESPONSE,
    NONE  // No new action, continues current action
};

enum class SocketType { BLOCKING, ASYNC };

/*
 * Indicates the type of the server we are running on.
 * Frontend receives requests from the internet, Backend receives from inside
 * the cluster.
 */
enum class ServerType { FRONTEND, BACKEND };

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

    virtual SocketState state() const = 0;

    virtual const Context& context() const = 0;
    virtual bool has_context() const = 0;

    /*
     * Returns the next logical action the socket will take if the given
     * operation is executed.
     */
    virtual SocketAction get_next_action(const SocketOperation op) const = 0;

    virtual SocketType type() const = 0;

    virtual ServerType server_type() const = 0;

    virtual bool is_context_processed() const = 0;
};

class AbstractSocketHandler : public SocketHandler {
   public:
    AbstractSocketHandler(int sockfd, const SocketState state,
                          const OriginalFunctions& orig);

    int fd() const override { return sockfd_; }

    SocketState state() const override { return state_; }

    const Context& context() const override {
        VERIFY(has_context(), "context() called when it is empty");
        return *context_;
    }

    bool has_context() const override { return static_cast<bool>(context_); }

    SocketType type() const override { return type_; }

    ServerType server_type() const override { return server_type_; }

    bool is_context_processed() const { return context_processed_; }

   protected:
    const int sockfd_;

    /*
     * Stores the current context. Initially empty.
     */
    std::unique_ptr<Context> context_;

    /*
     * Records the state of the socket.
     */
    SocketState state_;

    /*
     * Number of transactions that went through this socket.
     */
    int num_transactions_;

    /*
     * Indicates if the socket is blocking or non-blocking.
     */
    SocketType type_;

    ServerType server_type_;

    /*
     * Indicates if the context for the current transaction has been processed
     * yet or not.
     *
     * For a server socket it means that it has been read, for a client it means
     * that it has been sent.
     */
    bool context_processed_;

    const OriginalFunctions& orig_;
};
}
