#include "unittest.h"

#include "socket.h"


static const int FD = 19;

static const trace_id_t TRACE = 888;

static int test_socket_entry_new() {
    socket_entry_t *socket = socket_entry_new(FD, TRACE, SOCKET_OPENED);
    mu_assert(socket != NULL);

    mu_assert(socket->fd == FD);
    mu_assert(socket->trace == TRACE);
    mu_assert(socket->type == SOCKET_OPENED);
    mu_assert(socket->connid_set == 0);

    mu_assert(socket->parser->data == socket);

    return 0;
}

static int all_tests() {
    mu_run_test(test_socket_entry_new);
    return 0;
}
