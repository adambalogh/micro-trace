#include "gtest/gtest.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>

#include "fakeit.hpp"

#include "context.h"
#include "test_util.h"

using namespace microtrace;
using namespace fakeit;

static Mock<OriginalFunctions> mock;
static OriginalFunctions *orig_obj;

int last = 0;

namespace microtrace {
OriginalFunctions &orig() { return *orig_obj; }
}

int SERVER_PORT = 8543;
int DUMP_SERVER_PORT = 7354;

const char *const MSG = "aaaaaaaaaa";
const int MSG_LEN = 10;

void CreateMock() {
    mock.Reset();

    When(Method(mock, read)).AlwaysDo([](int fd, void *buf, size_t count) {
        return count;
    });
    When(Method(mock, write))
        .AlwaysDo([](int fd, const void *buf, size_t count) { return count; });
    When(Method(mock, accept))
        .AlwaysDo([](int sockfd, struct sockaddr *addr,
                     socklen_t *addrlen) -> auto { return ++last; });
    When(Method(mock, socket))
        .AlwaysDo([](int domain, int type, int protocol) -> auto {
            return ++last;
        });
    When(Method(mock, connect)).AlwaysReturn(0);
    When(Method(mock, close)).AlwaysReturn(0);

    orig_obj = &mock.get();
}

class TraceTest : public ::testing::Test {
   public:
    virtual void SetUp() { CreateMock(); }
};

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
TEST_F(TraceTest, PropagateTrace) {
    std::thread server_thread{[]() {
        EXPECT_TRUE(is_context_undefined());

        int ret;

        int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);

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
        Context zeroth_context = get_current_context();

        // Write response
        ret = write(first_client, &buf, MSG_LEN);

        // Start second transaction with first -- This should start a new trace
        ret = read(first_client, &buf, MSG_LEN);
        const Context first_context = get_current_context();
        EXPECT_FALSE(Context::IsSameTrace(zeroth_context, first_context));

        // Read second
        ret = read(second_client, &buf, MSG_LEN);
        const Context second_context = get_current_context();

        // Open connection to dump server
        // Note: we are in second_client's context now
        const int dump_client = CreateClientSocket(DUMP_SERVER_PORT);

        // Write to dump client
        ret = write(dump_client, &buf, MSG_LEN);
        EXPECT_EQ(second_context, get_current_context());

        // Read first
        ret = read(first_client, &buf, MSG_LEN);
        EXPECT_EQ(first_context, get_current_context());

        // Read dump client's response
        ret = read(dump_client, &buf, MSG_LEN);
        // Reading the response should have started a new span
        EXPECT_EQ(second_context.trace(), get_current_context().trace());
        EXPECT_NE(second_context.span(), get_current_context().span());
        EXPECT_EQ(second_context.span(), get_current_context().parent_span());

        close(server);
        close(first_client);
        close(second_client);
    }};

    server_thread.join();
}

TEST_F(TraceTest, BlockingConnectionPool) {
    std::thread server_thread{[this]() {
        int ret;

        // Create connection in advance, before any requests
        const int connection_pool = CreateClientSocket(DUMP_SERVER_PORT);

        int server = CreateServerSocket(SERVER_PORT);
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

        // Read request from first
        ret = read(first_client, &buf, MSG_LEN);
        const Context first_context = get_current_context();

        // Write to connection pool -- this should be associated with
        // first_context
        ret = write(connection_pool, &buf, MSG_LEN);
        EXPECT_EQ(first_context, get_current_context());

        // Read first half of request from second
        ret = read(second_client, &buf, MSG_LEN);
        const Context second_context = get_current_context();
        EXPECT_FALSE(Context::IsSameTrace(first_context, second_context));

        // Read response from connection pool -- this should set first_context
        ret = read(connection_pool, &buf, MSG_LEN);
        ASSERT_TRUE(Context::IsSameTrace(first_context, get_current_context()));

        // Read second half of request from second
        ret = read(second_client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        ASSERT_EQ(second_context, get_current_context());

        // Write to connection pool-- this is a new transaction and should be
        // associated with second_context
        ret = write(connection_pool, &buf, MSG_LEN);
        EXPECT_TRUE(
            Context::IsSameTrace(second_context, get_current_context()));
        EXPECT_FALSE(
            Context::IsSameTrace(first_context, get_current_context()));

        ASSERT_EQ(0, close(first_client));
        ASSERT_EQ(0, close(second_client));
        ASSERT_EQ(0, close(connection_pool));
        ASSERT_EQ(0, close(server));
    }};

    server_thread.join();
}

TEST_F(TraceTest, ContextSending) {
    std::thread server_thread{[]() {
        int ret;

        const int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);

        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(client, -1);

        char buf[MSG_LEN] = {'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'};

        // Read first
        ret = read(client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        const Context first_context = get_current_context();
        EXPECT_FALSE(first_context.is_zero());

        // Send response to third party
        const int dump_client = CreateClientSocket(DUMP_SERVER_PORT);
        ret = write(dump_client, &buf, MSG_LEN);

        Verify(
            // Context send
            Method(mock, write)
                .Matching([dump_client](int fd, const void *buf, size_t count) {
                    return fd == dump_client && count == sizeof(ContextStorage);
                }),

            // Buf send
            Method(mock, write)
                .Matching(
                    [dump_client, buf](int fd, const void *b, size_t count) {
                        return fd == dump_client && count == MSG_LEN;
                    }))
            .Exactly(Once);
        VerifyNoOtherInvocations(Method(mock, write));

    }};
    server_thread.join();
}

TEST_F(TraceTest, OnlySendContextToInternalService) {
    std::thread server_thread{[]() {
        int ret;

        const int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);

        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(client, -1);

        char buf[MSG_LEN];

        // Read first
        ret = read(client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        const Context first_context = get_current_context();
        EXPECT_FALSE(first_context.is_zero());

        // Open connection to "external" service
        // Send request to third party
        const int dump_client =
            CreateClientSocketIp("194.3.5.2", DUMP_SERVER_PORT);
        ret = write(dump_client, &buf, MSG_LEN);

        Verify(
            // Buf send
            Method(mock, write)
                .Matching(
                    [dump_client, buf](int fd, const void *b, size_t count) {
                        return fd == dump_client && count == MSG_LEN;
                    }))
            .Exactly(Once);
        VerifyNoOtherInvocations(Method(mock, write));

    }};
    server_thread.join();
}

TEST_F(TraceTest, BackendServerReadsContext) {
    // Set server type to backend so it reads the context
    putenv("MICROTRACE_SERVER_TYPE=backend");

    Context ctx;

    When(Method(mock, read))
        .Do([&ctx](int fd, void *buf, size_t count) {
            std::memcpy(buf, (void *)&ctx.storage(), sizeof(ContextStorage));
            return count;
        })
        .Do([](int fd, void *buf, size_t count) { return count; });
    orig_obj = &mock.get();

    std::thread server_thread{[&ctx]() {
        int ret;

        const int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);

        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(client, -1);

        char buf[MSG_LEN];

        // Read request
        ret = read(client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        const Context first_context = get_current_context();
        EXPECT_FALSE(first_context.is_zero());

        Verify(
            // Context read
            Method(mock, read)
                .Matching([client](int fd, const void *buf, size_t count) {
                    return fd == client && count == sizeof(ContextStorage);
                }),

            // Real read
            Method(mock, read)
                .Matching([client](int fd, const void *buf, size_t count) {
                    return fd == client && count == MSG_LEN;
                }))
            .Exactly(Once);

        VerifyNoOtherInvocations(Method(mock, read));

        // check that the context we sent is set as current_context
        EXPECT_TRUE(get_current_context().IsChildOf(ctx));

    }};
    server_thread.join();
}

TEST_F(TraceTest, FrontendServerDoesNotReadContext) {
    // Set server type to backend so it reads the context
    putenv("MICROTRACE_SERVER_TYPE=frontend");

    std::thread server_thread{[]() {
        int ret;

        const int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);

        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(client, -1);

        char buf[MSG_LEN];

        // Read request
        ret = read(client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        const Context first_context = get_current_context();
        EXPECT_FALSE(first_context.is_zero());

        Verify(
            // Real read
            Method(mock, read)
                .Matching([client](int fd, const void *buf, size_t count) {
                    return fd == client && count == MSG_LEN;
                }))
            .Exactly(Once);

        VerifyNoOtherInvocations(Method(mock, read));

    }};
    server_thread.join();
}
