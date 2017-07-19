#include <sys/socket.h>
#include <functional>
#include <memory>

#include "common.h"
#include "instrumented_socket.h"
#include "socket_handler.h"
#include "socket_interface.h"

namespace microtrace {

class ServerSocket : public InstrumentedSocket {
   public:
    ServerSocket(const ServerSocket &) = delete;

    ServerSocket(const int fd, std::unique_ptr<SocketHandler> handler,
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
    /*
     * Fills the given read buffer from the content of the unused
     * context_buffer.
     */
    ssize_t FillFromContextBuffer(void *buf, size_t len, ssize_t ret);

    ssize_t ReadContextIfNecessary();

    std::string context_buffer;
};
}
