#include "gtest/gtest.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "common.h"

const int SERVER_PORT = 3001;
const char *MSG = "aaaaaaaaaa";
const int MSG_LEN = 10;

class TraceTest : public ::testing::Test {
   protected:
    virtual void SetUp() { listening = false; }

    /*
     * Returns a socket that's bind to localhost:port
     */
    int CreateServerSocket(int port) {
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));

        int server = socket(AF_INET, SOCK_STREAM, 0);
        assert(server != -1);

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = port;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        int ret = bind(server, reinterpret_cast<struct sockaddr *>(&serv_addr),
                       sizeof(serv_addr));
        assert(ret == 0);
        return server;
    }

    std::mutex mu;
    std::condition_variable listen_cv;
    bool listening;
};

/*
 * This test verifires that the current_trace is set up and cleared correctly.
 *
 * First, we make sure that a trace is only set after the application has
 * read from a server socket. Then, after the server socket is closed, the
 * current_socket should be cleared.
 */
TEST_F(TraceTest, CurrentTrace) {
    EXPECT_EQ(UNDEFINED_TRACE, get_current_trace());

    std::thread t1{[this]() {
        struct sockaddr_in serv_addr, cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&serv_addr, 0, sizeof(serv_addr));
        memset(&cli_addr, 0, sizeof(cli_addr));

        int server = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_EQ(UNDEFINED_TRACE, get_current_trace());

        int ret;

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = SERVER_PORT;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        ret = bind(server, reinterpret_cast<struct sockaddr *>(&serv_addr),
                   sizeof(serv_addr));
        ASSERT_EQ(0, ret);
        EXPECT_EQ(UNDEFINED_TRACE, get_current_trace());

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

        trace_id_t trace = get_current_trace();
        EXPECT_NE(UNDEFINED_TRACE, trace);

        // server is not instrumented so it shouldn't change the trace
        ASSERT_EQ(0, close(server));
        EXPECT_EQ(trace, get_current_trace());

        // we don't clear the current trace if a related socket is closed
        ASSERT_EQ(0, close(client));
        EXPECT_EQ(trace, get_current_trace());
    }};

    int ret;

    struct sockaddr_in serv_addr;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = SERVER_PORT;

    // Wait until server is set up
    {
        std::unique_lock<std::mutex> l(mu);
        listen_cv.wait(l, [this]() { return listening == true; });
    }

    int client = socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_EQ(UNDEFINED_TRACE, get_current_trace());

    ret = connect(client, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    ASSERT_EQ(0, ret);

    // This socket wasn't opened in response to a request, so it's not tracked
    ret = write(client, MSG, MSG_LEN);
    EXPECT_EQ(UNDEFINED_TRACE, get_current_trace());

    close(client);
    EXPECT_EQ(UNDEFINED_TRACE, get_current_trace());

    t1.join();
}

/*
 * In this test, we make sure that whenever we successfully read from or write
 * to an instrumented socket, it's trace is set as the current trace
 */
TEST_F(TraceTest, TraceSwitch) {
    std::thread t1{[this]() {
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
        const trace_id_t first_trace = get_current_trace();
        EXPECT_NE(UNDEFINED_TRACE, first_trace);

        // Read second
        ret = read(second_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        const trace_id_t second_trace = get_current_trace();
        EXPECT_NE(UNDEFINED_TRACE, second_trace);
        EXPECT_NE(first_trace, second_trace);

        // Read first
        ret = read(first_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        ASSERT_EQ(first_trace, get_current_trace());

        // Write first
        ret = write(first_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        ASSERT_EQ(first_trace, get_current_trace());

        // Write second
        ret = write(second_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        ASSERT_EQ(second_trace, get_current_trace());

        // Write second
        ret = write(second_client, &buf, MSG_LEN);
        ASSERT_EQ(MSG_LEN, ret);
        ASSERT_EQ(second_trace, get_current_trace());

        close(server);
        close(first_client);
        close(second_client);
    }};

    int ret;

    struct sockaddr_in serv_addr;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = SERVER_PORT;

    // Wait until server is set up
    {
        std::unique_lock<std::mutex> l(mu);
        listen_cv.wait(l, [this]() { return listening == true; });
    }

    int first_client = socket(AF_INET, SOCK_STREAM, 0);
    ret =
        connect(first_client, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    ASSERT_EQ(0, ret);

    int second_client = socket(AF_INET, SOCK_STREAM, 0);
    ret = connect(second_client, (struct sockaddr *)&serv_addr,
                  sizeof(serv_addr));
    ASSERT_EQ(0, ret);

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

    ASSERT_EQ(UNDEFINED_TRACE, get_current_trace());

    close(first_client);
    close(second_client);

    t1.join();
}
