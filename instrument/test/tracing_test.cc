#include "gtest/gtest.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "context.h"

using namespace microtrace;

int SERVER_PORT = 8543;
const int DUMP_SERVER_PORT = 7354;

const char *const MSG = "aaaaaaaaaa";
const int MSG_LEN = 10;

class TraceTest : public ::testing::Test {
   protected:
    /*
     * A EchoServer is a TCP server that accepts connections, reads
     * MSG_LEN size chunks from them, and sends them back
     */
    class DumpServer {
       private:
        // Outgoing message
        struct Msg {
            Msg(std::string msg) : msg(msg), start(0) {}
            std::string msg;
            size_t start;
        };

       public:
        void Run() {
            int ret;
            std::vector<int> connections;

            int acceptor = CreateServerSocket(DUMP_SERVER_PORT);
            set_nonblock(acceptor);
            ret = listen(acceptor, 10);
            assert(ret == 0);

            {
                std::unique_lock<std::mutex> l;
                ready_ = true;
            }
            cv_.notify_all();

            std::map<int, std::queue<Msg>> out;

            char buf[MSG_LEN];
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
                    int ret = read(conn, &buf, MSG_LEN);
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
        int port_;
    };

   protected:
    virtual void SetUp() {
        ++SERVER_PORT;
        listening = false;
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
        assert(client != -1);
        int ret =
            connect(client, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        assert(ret == 0);
        return client;
    }

    /*
     * Returns a socket that's bind to localhost:port
     */
    static int CreateServerSocket(int port) {
        int ret;

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = port;
        serv_addr.sin_addr.s_addr = inet_addr("10.0.2.15");

        int server = socket(AF_INET, SOCK_STREAM, 0);
        assert(server != -1);

        int flags = fcntl(server, F_GETFL, 0);
        assert(flags >= 0);
        ret = fcntl(server, F_SETFL, flags | SO_REUSEADDR | SO_REUSEPORT);
        assert(ret >= 0);

        ret = bind(server, reinterpret_cast<struct sockaddr *>(&serv_addr),
                   sizeof(serv_addr));
        if (ret != 0) perror("CreateServerSocket");
        assert(ret == 0);
        return server;
    }

    std::mutex mu;
    std::condition_variable listen_cv;
    bool listening;
};

/*
 * This test verifies that the current_context is set up and cleared
 * correctly.
 *
 * First, we make sure that a context is only set after the application has
 * read from a server socket. Then, after the server socket is closed, the
 * current_socket should be cleared.
 */
TEST_F(TraceTest, CurrentContext) {
    EXPECT_TRUE(is_context_undefined());

    std::thread server_thread{[this]() {
        int ret;

        struct sockaddr_in serv_addr, cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&serv_addr, 0, sizeof(serv_addr));
        memset(&cli_addr, 0, sizeof(cli_addr));

        int server = CreateServerSocket(SERVER_PORT);
        EXPECT_TRUE(is_context_undefined());

        ret = listen(server, 5);
        ASSERT_EQ(0, ret);

        // Notify client that we are listening
        {
            std::unique_lock<std::mutex> l(mu);
            listening = true;
        }
        listen_cv.notify_all();

        int client = accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(client, -1);

        char buf[MSG_LEN];
        ret = read(client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);

        Context context = get_current_context();
        EXPECT_FALSE(is_context_undefined());

        // server is not instrumented so it shouldn't change the context
        ASSERT_EQ(0, close(server));
        EXPECT_EQ(context, get_current_context());

        // we don't clear the current context if a related socket is closed
        ASSERT_EQ(0, close(client));
        EXPECT_EQ(context, get_current_context());
    }};

    // Wait until server is set up
    {
        std::unique_lock<std::mutex> l(mu);
        listen_cv.wait(l, [this]() { return listening == true; });
    }

    int client = CreateClientSocket(SERVER_PORT);

    int ret;

    // This socket wasn't opened in response to a request, so it's not
    // tracked
    ret = write(client, MSG, MSG_LEN);
    EXPECT_EQ(ret, MSG_LEN);
    EXPECT_TRUE(is_context_undefined());

    close(client);
    EXPECT_TRUE(is_context_undefined());

    server_thread.join();
}

/*
 * In this test, we make sure that whenever we successfully read from or
 * write to an instrumented socket, it's context is set as the current
 * context
 */
TEST_F(TraceTest, TraceSwitch) {
    EXPECT_TRUE(is_context_undefined());

    std::thread server_thread{[this]() {
        int ret;

        int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);
        ASSERT_EQ(0, ret);

        // Notify client that we are listening
        {
            std::unique_lock<std::mutex> l(mu);
            listening = true;
        }
        listen_cv.notify_all();

        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        int first_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(first_client, -1);

        clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        int second_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(second_client, -1);

        char buf[MSG_LEN];

        // Read first
        ret = read(first_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_FALSE(is_context_undefined());
        const Context first_context = get_current_context();

        // Read second
        ret = read(second_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_FALSE(is_context_undefined());
        const Context second_context = get_current_context();
        EXPECT_NE(first_context, second_context);

        // Read first
        ret = read(first_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_EQ(first_context, get_current_context());

        // Write first
        ret = write(first_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_EQ(first_context, get_current_context());

        // Write second
        ret = write(second_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_EQ(second_context, get_current_context());

        // Write second
        ret = write(second_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_EQ(second_context, get_current_context());

        // Unsuccessful read first (client closed conn)
        ret = read(first_client, &buf, MSG_LEN);
        ASSERT_EQ(0, ret);
        EXPECT_EQ(first_context, get_current_context());

        // TODO figure out how to simulate write error
        // Unsuccessful write second
        // close(second_client);
        // ret = write(second_client, &buf, MSG_LEN);
        // ASSERT_EQ(-1, ret);
        // ASSERT_EQ(second_context, get_current_context());

        ASSERT_EQ(0, close(server));
        ASSERT_EQ(0, close(first_client));
    }};

    // Wait until server is set up
    {
        std::unique_lock<std::mutex> l(mu);
        listen_cv.wait(l, [this]() { return listening == true; });
    }

    int ret;

    int first_client = CreateClientSocket(SERVER_PORT);
    int second_client = CreateClientSocket(SERVER_PORT);

    // Write first
    ret = write(first_client, MSG, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    // Write second
    ret = write(second_client, MSG, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    // Write first
    ret = write(first_client, MSG, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    char buf[MSG_LEN];

    // Read first
    ret = read(first_client, &buf, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    // Read second
    ret = read(second_client, &buf, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    // Read second
    ret = read(second_client, &buf, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    EXPECT_TRUE(is_context_undefined());

    // Unsuccessful read first
    close(first_client);

    // Unsuccessful write second
    close(second_client);

    server_thread.join();
}

/*
 * In this test we make sure that the current_context is attached to sockets
 * that are opened in the application.
 *
 * There are 3 threads (servers) communicating with each other:
 *
 * local_thread <--> server_thread <--> dump_server_thread
 */
TEST_F(TraceTest, PropagateTrace) {
    EXPECT_TRUE(is_context_undefined());

    std::thread server_thread{[this]() {
        int ret;

        int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);
        ASSERT_EQ(0, ret);

        // Notify client that we are listening
        {
            std::unique_lock<std::mutex> l(mu);
            listening = true;
        }
        listen_cv.notify_all();

        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        int first_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(first_client, -1);

        clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        int second_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(second_client, -1);

        char buf[MSG_LEN];

        // Read first
        ret = read(first_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        const Context first_context = get_current_context();

        // Read second
        ret = read(second_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        const Context second_context = get_current_context();

        // Set up dump server
        DumpServer dump_server;
        std::thread dump_server_thread([&dump_server]() { dump_server.Run(); });
        {
            std::unique_lock<std::mutex> l(dump_server.mu());
            dump_server.cv().wait(
                l, [&dump_server]() { return dump_server.ready(); });
        }

        // Open connection to dump server
        // Note: we are in second_client's context now
        const int dump_client = CreateClientSocket(DUMP_SERVER_PORT);

        // Read first
        ret = read(first_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_EQ(first_context, get_current_context());

        // Write to dump client
        ret = write(dump_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_EQ(second_context, get_current_context());

        // Read first
        ret = read(first_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_EQ(first_context, get_current_context());

        // Read dump client's response
        ret = read(dump_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        EXPECT_EQ(second_context.trace(), get_current_context().trace());
        EXPECT_NE(second_context.span(), get_current_context().span());
        EXPECT_EQ(second_context.span(), get_current_context().parent_span());

        close(server);
        close(first_client);
        close(second_client);

        dump_server.shutdown();
        dump_server_thread.join();
    }};

    // Wait until server is set up
    {
        std::unique_lock<std::mutex> l(mu);
        listen_cv.wait(l, [this]() { return listening == true; });
    }

    int ret;

    int first_client = CreateClientSocket(SERVER_PORT);
    int second_client = CreateClientSocket(SERVER_PORT);

    // Write first
    ret = write(first_client, MSG, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    // Write second
    ret = write(second_client, MSG, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    // Write first
    ret = write(first_client, MSG, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    // Write first
    ret = write(first_client, MSG, MSG_LEN);
    ASSERT_EQ(MSG_LEN, ret);

    EXPECT_TRUE(is_context_undefined());

    close(first_client);
    close(second_client);
    server_thread.join();
}
