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

/*
 * In this test, we make sure that when a new transaction is started,
 * a context is created.
 */
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

        // we don't clear the current context if a related socket is closed
        ASSERT_EQ(0, close(server));
        EXPECT_EQ(context, get_current_context());
        ASSERT_EQ(0, close(client));
        EXPECT_EQ(context, get_current_context());
    }};
    server_thread.join();
}

/*
 * In this test, we make sure that whenever we successfully read from or
 * write to a socket, it's context is set as the current context.
 */
TEST_F(TraceTest, TraceSwitch) {
    std::thread server_thread{[]() {
        EXPECT_TRUE(is_context_undefined());

        const int server = CreateServerSocket(SERVER_PORT);
        listen(server, 5);

        // Accept first_client
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int first_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(first_client, -1);

        // Accept second_client
        clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int second_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(second_client, -1);

        char buf[MSG_LEN];

        // Read first_client
        read(first_client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        const Context first_context = get_current_context();

        // Read second_client
        read(second_client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        const Context second_context = get_current_context();
        EXPECT_NE(first_context, second_context);

        // Read first_client
        read(first_client, &buf, MSG_LEN);
        EXPECT_EQ(first_context, get_current_context());

        // Write first_client
        write(first_client, &buf, MSG_LEN);
        EXPECT_EQ(first_context, get_current_context());

        // Write second_client
        write(second_client, &buf, MSG_LEN);
        EXPECT_EQ(second_context, get_current_context());

        // Write second_client
        write(second_client, &buf, MSG_LEN);
        EXPECT_EQ(second_context, get_current_context());

        ASSERT_EQ(0, close(server));
        ASSERT_EQ(0, close(first_client));
    }};
    server_thread.join();
}

/*
 * In this test, we test several functionalities, including context switching,
 * creating new spans at the start of new transactions and attaching the current
 * context to newly created client sockets.
 */
TEST_F(TraceTest, SampleApplication) {
    std::thread server_thread{[]() {
        EXPECT_TRUE(is_context_undefined());

        int server = CreateServerSocket(SERVER_PORT);
        listen(server, 5);

        // Accept first_client
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        int first_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(first_client, -1);

        // Accept second_client
        clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        int second_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(second_client, -1);

        char buf[MSG_LEN];

        // Read from first_client -- this is the first transaction with
        // first_client
        read(first_client, &buf, MSG_LEN);
        Context zeroth_context = get_current_context();

        // Write response to first_client
        write(first_client, &buf, MSG_LEN);
        write(first_client, &buf, MSG_LEN);

        // Read from first_client -- this is the start of the the second
        // transaction with first_client, and should start a
        // new trace
        read(first_client, &buf, MSG_LEN);
        const Context first_context = get_current_context();
        EXPECT_FALSE(Context::IsSameTrace(zeroth_context, first_context));

        // Read from second_client
        read(second_client, &buf, MSG_LEN);
        const Context second_context = get_current_context();

        // Open connection to dump server
        // Note: we are in second_client's context now
        const int dump_client = CreateClientSocket(DUMP_SERVER_PORT);

        // Write to dump_client
        write(dump_client, &buf, MSG_LEN);
        EXPECT_EQ(second_context, get_current_context());

        // Read from first_client
        read(first_client, &buf, MSG_LEN);
        EXPECT_EQ(first_context, get_current_context());

        // Read dump_client's response
        read(dump_client, &buf, MSG_LEN);

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

        // Create connection in advance, before any requests have arrived
        const int connection_pool = CreateClientSocket(DUMP_SERVER_PORT);

        int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);

        // Accept first_client
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int first_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(first_client, -1);

        // Accept second_client
        clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int second_client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(second_client, -1);

        char buf[MSG_LEN];

        // Read request from first_client
        ret = read(first_client, &buf, MSG_LEN);
        const Context first_context = get_current_context();

        // Write to connection in connection pool -- this should be associated
        // with first_context
        ret = write(connection_pool, &buf, MSG_LEN);
        EXPECT_EQ(first_context, get_current_context());

        // Read first half of request from second_client
        ret = read(second_client, &buf, MSG_LEN);
        const Context second_context = get_current_context();
        EXPECT_FALSE(Context::IsSameTrace(first_context, second_context));

        // Read response from connection pool -- this should be part of
        // first_context's trace
        ret = read(connection_pool, &buf, MSG_LEN);
        ASSERT_TRUE(Context::IsSameTrace(first_context, get_current_context()));

        // Read second half of request from second_client
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

        close(first_client);
        close(second_client);
        close(connection_pool);
        close(server);
    }};

    server_thread.join();
}

/*
 * This test verifiess that the context is attached to all outgoing messages
 * being sent to other internal microservices.
 */
TEST_F(TraceTest, ContextIsSentToInternalServices) {
    const std::string internal_service_ip = "10.0.2.15";

    // In Kubernetes, all internal serices have their IP address set as an
    // environment variable on all machines. This indicates that it is an
    // internal server
    putenv(const_cast<char *>(
        ("DUMP_SERVICE_HOST=" + internal_service_ip).c_str()));

    std::thread server_thread{[&internal_service_ip]() {
        int ret;

        const int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);

        // Acccept first_client
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        const int client =
            accept(server, (struct sockaddr *)&cli_addr, &clilen);
        ASSERT_GT(client, -1);

        char buf[MSG_LEN] = {'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'};

        // Read from first_client
        ret = read(client, &buf, MSG_LEN);
        EXPECT_FALSE(is_context_undefined());
        const Context first_context = get_current_context();
        EXPECT_FALSE(first_context.is_zero());

        // Send request to an "internal" microservice
        const int dump_client =
            CreateClientSocketIp(internal_service_ip, DUMP_SERVER_PORT);
        ret = write(dump_client, &buf, MSG_LEN);

        Verify(
            // Context is sent first
            Method(mock, write)
                .Matching([dump_client](int fd, const void *buf, size_t count) {
                    return fd == dump_client && count == sizeof(ContextStorage);
                }),

            // Next, the actual message
            Method(mock, write)
                .Matching(
                    [dump_client, buf](int fd, const void *b, size_t count) {
                        return fd == dump_client && count == MSG_LEN;
                    }))
            .Exactly(Once);

        // Nothing else is sent
        VerifyNoOtherInvocations(Method(mock, write));

    }};
    server_thread.join();
}

/*
 * In this test, we verify that the context is not sent to external services.
 */
TEST_F(TraceTest, ContextIsNOTSentToExternalServices) {
    std::thread server_thread{[]() {
        int ret;

        const int server = CreateServerSocket(SERVER_PORT);
        ret = listen(server, 5);

        // Accept client
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

        // Open connection to "external" server. Note that no environment
        // variable has been set with this IP address, so it should be
        // considered an external server
        const int dump_client =
            CreateClientSocketIp("194.3.5.2", DUMP_SERVER_PORT);
        ret = write(dump_client, &buf, MSG_LEN);

        Verify(
            // Only the message is sent, no context
            Method(mock, write)
                .Matching(
                    [dump_client, buf](int fd, const void *b, size_t count) {
                        return fd == dump_client && count == MSG_LEN;
                    }))
            .Exactly(Once);
        // Nothing else was sent
        VerifyNoOtherInvocations(Method(mock, write));

    }};
    server_thread.join();
}

/*
 * In this test, we verify that backend servers read the context first at the
 * start of every new transaction, and use that context as current_context.
 */
TEST_F(TraceTest, BackendServerReadsContext) {
    // Set server type to backend so it reads the context
    putenv("MICROTRACE_SERVER_TYPE=backend");

    // Create a random context
    Context ctx;

    // Set up read, so it first returns the context, and next the message
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

        // Accept client
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
            // Verify that the context is read first
            Method(mock, read)
                .Matching([client](int fd, const void *buf, size_t count) {
                    return fd == client && count == sizeof(ContextStorage);
                }),

            // Next, the actual message is read
            Method(mock, read)
                .Matching([client](int fd, const void *buf, size_t count) {
                    return fd == client && count == MSG_LEN;
                }))
            .Exactly(Once);

        // No other read
        VerifyNoOtherInvocations(Method(mock, read));

        // check that the context we sent is set as current_context, and a new
        // span has been started
        EXPECT_TRUE(Context::IsSameTrace(ctx, get_current_context()));
        EXPECT_TRUE(get_current_context().IsChildOf(ctx));

    }};
    server_thread.join();
}

/*
 * In this test we verify that frontend servers do not try to read the context
 * at the start of every new transaction.
 */
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

        // No other read
        VerifyNoOtherInvocations(Method(mock, read));

    }};
    server_thread.join();
}
