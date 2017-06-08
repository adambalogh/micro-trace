#pragma once

#include "instrumented_socket.h"

namespace microtrace {

class ServerSocket : public AbstractInstrumentedSocket {
   public:
    ServerSocket(int sockfd);

    ssize_t Read(const void* buf, size_t len, IoFunction fun) override;
    ssize_t Write(const struct iovec* iov, int iovcnt, IoFunction fun) override;
    int Close(CloseFunction fun) override;

   private:
    trace_id_t trace_;
};
}
