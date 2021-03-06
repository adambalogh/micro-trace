#include <sys/socket.h>
#include <functional>
#include <memory>

#include "client_socket_handler.h"
#include "common.h"
#include "instrumented_socket.h"
#include "socket_interface.h"

namespace microtrace {

class ClientSocket : public InstrumentedSocket {
   public:
    ClientSocket(const ClientSocket &) = delete;

    ClientSocket(const int fd, std::unique_ptr<ClientSocketHandler> handler,
                 const OriginalFunctions &orig);

    void Async() override;
    void Connected(const std::string &ip);

    ssize_t RecvFrom(void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen) override;
    ssize_t Recv(void *buf, size_t len, int flags) override;
    ssize_t Read(void *buf, size_t count) override;
    ssize_t Write(const void *buf, size_t count) override;
    ssize_t Writev(const struct iovec *iov, int iovcnt) override;
    ssize_t Send(const void *buf, size_t len, int flags) override;
    ssize_t SendTo(const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr,
                   socklen_t addrlen) override;
    ssize_t SendMsg(const struct msghdr *msg, int flags) override;

    int Close() override;

   private:
    std::unique_ptr<ClientSocketHandler> handler_;
};
}
