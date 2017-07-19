#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "instrumented_socket.h"
#include "orig_functions.h"

using namespace microtrace;

// set socket to non-blocking
void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    assert(flags >= 0);
    int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    assert(ret >= 0);
}

/*
 * Returns a socket that's connected to localhost:port
 */
static int CreateClientSocket(int port) {
    struct sockaddr_in serv_addr;
    serv_addr.sin_addr.s_addr = inet_addr("10.0.2.15");
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = port;

    int client = socket(AF_INET, SOCK_STREAM, 0);
    connect(client, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    return client;
}

/*
 * Returns a socket that's bind to localhost:port
 */
static int CreateServerSocket(int port) {
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = port;
    serv_addr.sin_addr.s_addr = inet_addr("10.0.2.15");

    int server = socket(AF_INET, SOCK_STREAM, 0);
    assert(server != -1);

    int flags = fcntl(server, F_GETFL, 0);
    fcntl(server, F_SETFL, flags | SO_REUSEADDR | SO_REUSEPORT);
    bind(server, reinterpret_cast<struct sockaddr *>(&serv_addr),
         sizeof(serv_addr));
    return server;
}

/*
 * A EchoServer is a TCP server that accepts connections, reads
 * MSG_LEN size chunks from them, and sends them back
 */
class EchoServer {
   private:
    // Outgoing message
    struct Msg {
        Msg(std::string msg) : msg(msg), start(0) {}
        std::string msg;
        size_t start;
    };

   public:
    EchoServer(int port, int msg_len) : port_(port), msg_len_(msg_len) {}

    void Run() {
        int ret;
        std::vector<int> connections;

        int acceptor = CreateServerSocket(port_);
        set_nonblock(acceptor);
        ret = listen(acceptor, 10);
        assert(ret == 0);

        {
            std::unique_lock<std::mutex> l;
            ready_ = true;
        }
        cv_.notify_all();

        std::map<int, std::queue<Msg>> out;

        char buf[msg_len_];
        while (true) {
            {
                std::unique_lock<std::mutex> l(mu_);
                if (shutdown_) break;
            }
            struct sockaddr_in cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            memset(&cli_addr, 0, sizeof(cli_addr));
            int client =
                accept(acceptor, (struct sockaddr *)&cli_addr, &clilen);
            if (client != -1) {
                set_nonblock(client);
                connections.push_back(client);
            }

            for (const int conn : connections) {
                int ret = read(conn, &buf[0], msg_len_);
                if (ret > 0) {
                    std::string msg(&buf[0], ret);
                    out[conn].push(Msg{msg});
                }
            }

            for (auto &it : out) {
                if (it.second.empty()) continue;

                Msg &m = it.second.front();
                int ret = write(it.first, m.msg.c_str() + m.start,
                                m.msg.size() - m.start);
                if (ret > 0) m.start += ret;
                if (m.start == m.msg.size()) {
                    it.second.pop();
                }
            }
        }

        // Shutdown
        ret = close(acceptor);
        assert(ret == 0);
        for (const int conn : connections) {
            close(conn);
        }
    }

    std::condition_variable &cv() { return cv_; }
    std::mutex &mu() { return mu_; }
    bool ready() const { return ready_; }

    void shutdown() {
        std::unique_lock<std::mutex> l(mu_);
        shutdown_ = true;
    }

   private:
    bool ready_ = false;
    bool shutdown_ = false;
    std::mutex mu_;
    std::condition_variable cv_;
    const int port_;
    const int msg_len_;
};

class EmptyOriginalFunctions : public OriginalFunctions {
   private:
    mutable int last_socket = 0;

   public:
    int socket(int domain, int type, int protocol) const {
        return ++last_socket;
    }
    int connect(int sockfd, const struct sockaddr *addr,
                socklen_t addrlen) const {
        return 0;
    }
    int close(int fd) const { return 0; }
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) const {
        return ++last_socket;
    }
    int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
                int flags) const {
        return ++last_socket;
    }
    ssize_t recv(int sockfd, void *buf, size_t len, int flags) const {
        return len;
    }
    ssize_t read(int fd, void *buf, size_t count) const { return count; }
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen) const {
        return len;
    }
    ssize_t write(int fd, const void *buf, size_t count) const { return count; }
    ssize_t writev(int fd, const struct iovec *iov, int iovcnt) const {
        return 11;
    }
    ssize_t send(int sockfd, const void *buf, size_t len, int flags) const {
        return len;
    }
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen) const {
        return len;
    }
    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) const {
        return 11;
    }
    int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
                 int flags) const {
        return 11;
    }
    int uv_tcp_connect(uv_connect_t *req, uv_tcp_t *handle,
                       const struct sockaddr *addr, uv_connect_cb cb) const {
        return 0;
    }
    int uv_accept(uv_stream_t *server, uv_stream_t *client) const {
        client->io_watcher.fd = ++last_socket;
        return 0;
    }
    int uv_getaddrinfo(uv_loop_t *loop, uv_getaddrinfo_t *req,
                       uv_getaddrinfo_cb getaddrinfo_cb, const char *node,
                       const char *service,
                       const struct addrinfo *hints) const {
        return 0;
    }
    int getpeername(int sockfd, struct sockaddr *addr,
                    socklen_t *addrlen) const {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
        addr_in->sin_family = AF_INET;
        addr_in->sin_port = 543;
        addr_in->sin_addr.s_addr = inet_addr("10.3.2.15");
        return 0;
    }

    int getsockname(int sockfd, struct sockaddr *addr,
                    socklen_t *addrlen) const {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
        addr_in->sin_family = AF_INET;
        addr_in->sin_port = 10;
        addr_in->sin_addr.s_addr = inet_addr("10.0.1.43");
        return 0;
    }
};

class DumbSocketHandler : public SocketHandler {
   public:
    static std::unique_ptr<SocketHandler> New(int fd, const SocketRole role) {
        return std::make_unique<DumbSocketHandler>(fd, role);
    }

    DumbSocketHandler(int fd, const SocketRole role) : fd_(fd), role_(role) {}

    void Async() {}

    Result BeforeRead(const void *buf, size_t len) { return Result::Ok; }
    void AfterRead(const void *buf, size_t len, ssize_t ret) {}

    Result BeforeWrite(const struct iovec *iov, int iovcnt) {
        return Result::Ok;
    }
    void AfterWrite(const struct iovec *iov, int iovcnt, ssize_t ret) {}

    Result BeforeClose() { return Result::Ok; }
    void AfterClose(int ret) {}

    int fd() const { return fd_; }

    SocketRole role() const { return role_; }
    bool role_server() const override { return role_ == SocketRole::SERVER; }
    bool role_client() const override { return role_ == SocketRole::CLIENT; }

   private:
    int fd_;
    SocketRole role_;
};
