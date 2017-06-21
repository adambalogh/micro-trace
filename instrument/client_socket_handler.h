#pragma once

#include "socket_handler.h"

#include <chrono>

namespace microtrace {

struct RequestLogWrapper;

class ClientSocketHandler : public AbstractSocketHandler {
   private:
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
            return std::chrono::duration_cast<std::chrono::duration<double>>(
                       end_ - start_)
                .count();
        }

       private:
        std::chrono::time_point<std::chrono::system_clock> system_start_;
        std::chrono::time_point<std::chrono::steady_clock> start_;
        std::chrono::time_point<std::chrono::steady_clock> end_;
    };

   public:
    ClientSocketHandler(int sockfd);

    virtual Result BeforeRead(const void* buf, size_t len) override;
    virtual void AfterRead(const void* buf, size_t len, ssize_t ret) override;

    virtual Result BeforeWrite(const struct iovec* iov, int iovcnt) override;
    virtual void AfterWrite(const struct iovec* iov, int iovcnt,
                            ssize_t ret) override;

    virtual Result BeforeClose() override;
    virtual void AfterClose(int ret) override;

    bool has_txn() const { return static_cast<bool>(txn_); }

   private:
    void FillRequestLog(RequestLogWrapper& log);

    /*
     * Represents the current transaction that is going through this socket.
     * It is initially empty.
     */
    std::unique_ptr<Transaction> txn_;
};
}
