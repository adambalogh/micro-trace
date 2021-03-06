#pragma once

#include "http_processor.h"
#include "socket_handler.h"
#include "trace_logger.h"

namespace microtrace {

struct RequestLogWrapper;

class ServiceIpMap {
   public:
    ServiceIpMap();

    /*
     * Returns the service name belonging to the given ip address.
     *
     * If no service is found, it returns a nullptr.
     */
    const std::string* Get(const std::string& ip);

   private:
    /*
     * Maps ip addresses to Kubernetes service names
     */
    std::unordered_map<std::string, std::string> map_;
};

class ClientSocketHandler : public AbstractSocketHandler {
   public:
    ClientSocketHandler(int sockfd, const OriginalFunctions& orig)
        : AbstractSocketHandler(sockfd, SocketState::WILL_WRITE, orig) {}

    virtual void HandleConnect(const std::string& ip) = 0;
    virtual bool has_txn() const = 0;
};

class ClientSocketHandlerImpl : public ClientSocketHandler {
   public:
    ClientSocketHandlerImpl(int sockfd, TraceLogger* trace_logger,
                            const OriginalFunctions& orig);

    virtual void Async() override;
    void HandleConnect(const std::string& ip) override;

    virtual Result BeforeRead(const void* buf, size_t len) override;
    virtual void AfterRead(const void* buf, size_t len, ssize_t ret) override;

    virtual Result BeforeWrite(const struct iovec* iov, int iovcnt) override;
    virtual void AfterWrite(const struct iovec* iov, int iovcnt,
                            ssize_t ret) override;

    virtual Result BeforeClose() override;
    virtual void AfterClose(int ret) override;

    virtual SocketAction get_next_action(
        const SocketOperation op) const override;

    bool has_txn() const override { return static_cast<bool>(txn_); }

   private:
    int SetConnection();

    /*
     * Sends the current context to the other end.
     *
     * Returns true if the trace was successfully written to TCP buffer.
     */
    bool SendContext();

    bool SendContextBlocking();
    bool SendContextAsync();

    /*
     * Sends the context only if it is the beginning of a new request.
     *
     * Returns false if the context was necessary to send but was
     * unsuccessful.
     */
    bool SendContextIfNecessary();

    void FillRequestLog(RequestLogWrapper& log);

    /*
     * The connection this socket represents. Remains the same throughout
     * the socket's lifetime, and becomes invalid after Close() has been
     * called.
     */
    Connection conn_;

    /*
     * Stores IP addesses to service names.
     *
     * Safe to use from multiple threads.
     */
    static ServiceIpMap service_map_;

    /*
     * Represents the current transaction that is going through this socket.
     * It is initially empty.
     */
    std::unique_ptr<Transaction> txn_;

    TraceLogger* const trace_logger_;

    /*
     * Indicates if this socket is connected to another pod in Kubernetes.
     */
    bool kubernetes_socket_;

    HttpProcessor http_processor_;
};
}
