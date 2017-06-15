#include "gtest/gtest.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>

#include "context.h"
#include "test_util.h"

using namespace microtrace;

namespace microtrace {
OriginalFunctions &orig() {
    static EmptyOriginalFunctions o;
    return o;
}
}

int SERVER_PORT = 8543;
int DUMP_SERVER_PORT = 7354;

const char *const MSG = "aaaaaaaaaa";
const int MSG_LEN = 10;

class TraceTest : public ::testing::Test {};

TEST_F(TraceTest, CurrentContext) {
    std::thread server_thread{[]() {
        EXPECT_TRUE(is_context_undefined());

        int ret;

        struct sockaddr_in serv_addr, cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&serv_addr, 0, sizeof(serv_addr));
        memset(&cli_addr, 0, sizeof(cli_addr));

        // Create server
        const int server = CreateServerSocket(SERVER_PORT);
        EXPECT_TRUE(is_context_undefined());

        // Accept connection
        ret = listen(server, 5);
        const int client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(client, -1);

        // Read request
        char buf[MSG_LEN];
        ret = read(client, &buf, MSG_LEN);

        // Context must be set
        EXPECT_FALSE(is_context_undefined());
        const Context context = get_current_context();

        ASSERT_EQ(0, close(server));
        EXPECT_EQ(context, get_current_context());

        // we don't clear the current context if a related socket is closed
        ASSERT_EQ(0, close(client));
        EXPECT_EQ(context, get_current_context());
    }};
    server_thread.join();
}

/*
 * In this test, we make sure that whenever we successfully read from or
 * write to an instrumented socket, it's context is set as the current
 * context
 */
TEST_F(TraceTest, TraceSwitch) {
    std::thread server_thread{[]() {
        EXPECT_TRUE(is_context_undefined());

        int ret;

        const int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);

        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int first_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(first_client, -1);

        clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int second_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(second_client, -1);

        char buf[MSG_LEN];

        // Read first
        ret = read(first_client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        const Context first_context = get_current_context();

        // Read second
        ret = read(second_client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        const Context second_context = get_current_context();
        EXPECT_NE(first_context, second_context);

        // Read first
        ret = read(first_client, &buf, MSG_LEN);
        EXPECT_EQ(first_context, get_current_context());

        // Write first
        ret = write(first_client, &buf, MSG_LEN);
        EXPECT_EQ(first_context, get_current_context());

        // Write second
        ret = write(second_client, &buf, MSG_LEN);
        EXPECT_EQ(second_context, get_current_context());

        // Write second
        ret = write(second_client, &buf, MSG_LEN);
        EXPECT_EQ(second_context, get_current_context());

        // Unsuccessful read first (client closed conn)
        // ret = read(first_client, &buf, MSG_LEN);
        // ASSERT_EQ(0, ret);
        // EXPECT_EQ(first_context, get_current_context());

        // TODO figure out how to simulate write error
        // Unsuccessful write second
        // close(second_client);
        // ret = write(second_client, &buf, MSG_LEN);
        // ASSERT_EQ(-1, ret);
        // ASSERT_EQ(second_context, get_current_context());

        ASSERT_EQ(0, close(server));
        ASSERT_EQ(0, close(first_client));
    }};
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
// TEST_F(TraceTest, PropagateTrace) {
//    EXPECT_TRUE(is_context_undefined());

//    std::thread server_thread{[this]() {
//        int ret;

//        int server = CreateServerSocket(SERVER_PORT);
//        ret = listen(server, 5);
//        ASSERT_EQ(0, ret);

//        // Notify client that we are listening
//        {
//            std::unique_lock<std::mutex> l(mu);
//            listening = true;
//        }
//        listen_cv.notify_all();

//        struct sockaddr_in cli_addr;
//        socklen_t clilen = sizeof(cli_addr);
//        memset(&cli_addr, 0, sizeof(cli_addr));
//        int first_client =
//            accept(server, (struct sockaddr *)&cli_addr, &clilen);
//        ASSERT_GT(first_client, -1);

//        clilen = sizeof(cli_addr);
//        memset(&cli_addr, 0, sizeof(cli_addr));
//        int second_client =
//            accept(server, (struct sockaddr *)&cli_addr, &clilen);
//        ASSERT_GT(second_client, -1);

//        char buf[MSG_LEN];

//        // Read first
//        ret = read(first_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        const Context first_context = get_current_context();

//        // Read second
//        ret = read(second_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        const Context second_context = get_current_context();

//        // Set up dump server
//        DumpServer dump_server;
//        std::thread dump_server_thread([&dump_server]() { dump_server.Run();
//        });
//        {
//            std::unique_lock<std::mutex> l(dump_server.mu());
//            dump_server.cv().wait(
//                l, [&dump_server]() { return dump_server.ready(); });
//        }

//        // Open connection to dump server
//        // Note: we are in second_client's context now
//        const int dump_client = CreateClientSocket(DUMP_SERVER_PORT);

//        // Read first
//        ret = read(first_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        EXPECT_EQ(first_context, get_current_context());

//        // Write to dump client
//        ret = write(dump_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        EXPECT_EQ(second_context, get_current_context());

//        // Read first
//        ret = read(first_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        EXPECT_EQ(first_context, get_current_context());

//        // Read dump client's response
//        ret = read(dump_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        EXPECT_EQ(second_context.trace(), get_current_context().trace());
//        EXPECT_NE(second_context.span(), get_current_context().span());
//        EXPECT_EQ(second_context.span(), get_current_context().parent_span());

//        close(server);
//        close(first_client);
//        close(second_client);

//        dump_server.shutdown();
//        dump_server_thread.join();
//    }};

//    // Wait until server is set up
//    {
//        std::unique_lock<std::mutex> l(mu);
//        listen_cv.wait(l, [this]() { return listening == true; });
//    }

//    int ret;

//    int first_client = CreateClientSocket(SERVER_PORT);
//    int second_client = CreateClientSocket(SERVER_PORT);

//    // Write first
//    ret = write(first_client, MSG, MSG_LEN);
//    ASSERT_EQ(MSG_LEN, ret);

//    // Write second
//    ret = write(second_client, MSG, MSG_LEN);
//    ASSERT_EQ(MSG_LEN, ret);

//    // Write first
//    ret = write(first_client, MSG, MSG_LEN);
//    ASSERT_EQ(MSG_LEN, ret);

//    // Write first
//    ret = write(first_client, MSG, MSG_LEN);
//    ASSERT_EQ(MSG_LEN, ret);

//    EXPECT_TRUE(is_context_undefined());

//    close(first_client);
//    close(second_client);
//    server_thread.join();
//}

// TEST_F(TraceTest, BlockingConnectionPool) {
//    std::thread server_thread{[this]() {
//        int ret;

//        // Set up dump server
//        DumpServer dump_server;
//        std::thread dump_server_thread([&dump_server]() { dump_server.Run();
//        });
//        {
//            std::unique_lock<std::mutex> l(dump_server.mu());
//            dump_server.cv().wait(
//                l, [&dump_server]() { return dump_server.ready(); });
//        }
//        int server = CreateServerSocket(SERVER_PORT);
//        ret = listen(server, 5);
//        ASSERT_EQ(0, ret);

//        // Notify client that we are listening
//        {
//            std::unique_lock<std::mutex> l(mu);
//            listening = true;
//        }
//        listen_cv.notify_all();

//        struct sockaddr_in cli_addr;
//        socklen_t clilen = sizeof(cli_addr);
//        memset(&cli_addr, 0, sizeof(cli_addr));
//        const int first_client =
//            accept(server, (struct sockaddr *)&cli_addr, &clilen);
//        ASSERT_GT(first_client, -1);

//        clilen = sizeof(cli_addr);
//        memset(&cli_addr, 0, sizeof(cli_addr));
//        const int second_client =
//            accept(server, (struct sockaddr *)&cli_addr, &clilen);
//        ASSERT_GT(second_client, -1);

//        char buf[MSG_LEN];

//        // Create connection in advance, before any requests
//        const int dump_client = CreateClientSocket(DUMP_SERVER_PORT);

//        // Read request from first
//        ret = read(first_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        EXPECT_FALSE(is_context_undefined());
//        const Context first_context = get_current_context();

//        // Write to dump server -- this should be associated with
//        first_context
//        ret = write(dump_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        EXPECT_EQ(first_context, get_current_context());

//        // Read first half of request from second
//        ret = read(second_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        EXPECT_FALSE(is_context_undefined());
//        const Context second_context = get_current_context();

//        // Read from dump server -- this should set first_context
//        ret = read(dump_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        ASSERT_EQ(first_context, get_current_context());

//        // Read second half of request from second
//        ret = read(second_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        EXPECT_FALSE(is_context_undefined());
//        ASSERT_EQ(second_context, get_current_context());

//        // Write to dump server -- this is a new transaction and should be
//        // associated with second_context
//        ret = write(dump_client, &buf, MSG_LEN);
//        ASSERT_EQ(MSG_LEN, ret);
//        EXPECT_EQ(second_context, get_current_context());

//        dump_server.shutdown();
//        dump_server_thread.join();

//        ASSERT_EQ(0, close(first_client));
//        ASSERT_EQ(0, close(second_client));
//        ASSERT_EQ(0, close(dump_client));
//        ASSERT_EQ(0, close(server));
//    }};

//    // Wait until server is set up
//    {
//        std::unique_lock<std::mutex> l(mu);
//        listen_cv.wait(l, [this]() { return listening == true; });
//    }

//    int ret;
//    const int first_client = CreateClientSocket(SERVER_PORT);
//    const int second_client = CreateClientSocket(SERVER_PORT);

//    // Write first request
//    ret = write(first_client, MSG, MSG_LEN);
//    EXPECT_EQ(ret, MSG_LEN);
//    EXPECT_TRUE(is_context_undefined());

//    // Write first half of second request
//    ret = write(second_client, MSG, MSG_LEN);
//    EXPECT_EQ(ret, MSG_LEN);
//    EXPECT_TRUE(is_context_undefined());

//    // Write second half of second request
//    ret = write(second_client, MSG, MSG_LEN);
//    EXPECT_EQ(ret, MSG_LEN);
//    EXPECT_TRUE(is_context_undefined());

//    server_thread.join();
//}
