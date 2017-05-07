#include "unittest.h"

#include "socket.h"

static const int FD = 19;

static const trace_id_t TRACE = 888;

static int test_add() {
    socket_entry_t *socket = socket_entry_new(FD, TRACE, SOCKET_OPENED);

    mu_assert(socket->fd == FD);
    mu_assert(socket->trace == TRACE);
    mu_assert(socket->type == SOCKET_OPENED);
    mu_assert(1 == 2);

    return 0;
}

static int all_tests() {
    mu_run_test(test_add);
    return 0;
}
