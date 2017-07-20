#include <sys/socket.h>
#include <functional>
#include <memory>

#include "common.h"
#include "instrumented_socket.h"
#include "server_socket_handler.h"
#include "socket_interface.h"

namespace microtrace {

class ServerSocket : public InstrumentedSocket {
   public:
    ServerSocket(const ServerSocket &) = delete;

    ServerSocket(const int fd, std::unique_ptr<ServerSocketHandler> handler,
                 const OriginalFunctions &orig);

    void Async() override;

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
    ssize_t ReadContextIfNecessary();

    ssize_t ReadContextBlocking();
    ssize_t ReadContextAsync();

    std::unique_ptr<ServerSocketHandler> handler_;

    // Buffer for reading context bytes
    std::string context_buffer;
};
}
