#pragma once

#include "instrumented_socket.h"

namespace microtrace {

struct RequestLogWrapper;

class ClientSocket : public AbstractInstrumentedSocket {
   private:
    struct Transaction {
        time_t start;
        time_t end;
    };

   public:
    ClientSocket(int sockfd, const trace_id_t trace);

    ssize_t Read(const void* buf, size_t len, IoFunction fun) override;
    ssize_t Write(const struct iovec* iov, int iovcnt, IoFunction fun) override;
    int Close(CloseFunction fun) override;

    bool has_txn() const { return static_cast<bool>(txn_); }

   private:
    void FillRequestLog(RequestLogWrapper& log);

    std::unique_ptr<Transaction> txn_;
};
}
