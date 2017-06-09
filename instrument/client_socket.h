#pragma once

#include "instrumented_socket.h"

#include <chrono>

namespace microtrace {

struct RequestLogWrapper;

class ClientSocket : public AbstractInstrumentedSocket {
   private:
    /*
     * A transaction is a request-respone sequence between a client and
     * server.
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
    ClientSocket(int sockfd, const trace_id_t trace);

    ssize_t Read(const void* buf, size_t len, IoFunction fun) override;
    ssize_t Write(const struct iovec* iov, int iovcnt, IoFunction fun) override;
    int Close(CloseFunction fun) override;

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
