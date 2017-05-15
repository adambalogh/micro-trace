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

std::mutex mu;
std::condition_variable listen_cv;
bool listening = false;

/*
 * This test verifires that the current_trace is set up and cleared correctly.
 *
 * First, we make sure that a trace is only set after the application has
 * read from a server socket. Then, after the server socket is closed, the
 * current_socket should be cleared.
 */
TEST(Trace, CurrentTrace) {
    EXPECT_EQ(UNDEFINED_TRACE, get_current_trace());

    std::thread t1{[]() {
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
        ASSERT_GT(ret, -1);
        EXPECT_EQ(UNDEFINED_TRACE, get_current_trace());

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
        listen_cv.wait(l, []() { return listening == true; });
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
