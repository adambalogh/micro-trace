#include "benchmark/benchmark.h"

#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>

#include "test_util.h"

static void SocketCreateClose(benchmark::State& state) {
    while (state.KeepRunning()) {
        int s = socket(AF_INET, SOCK_DGRAM, 0);
        close(s);
    }
}
BENCHMARK(SocketCreateClose);

static void SocketWrite(benchmark::State& state) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    set_nonblock(s);
    char buf[100];
    int LEN = 100;
    assert(read(s, &buf[0], LEN) == -1);

    while (state.KeepRunning()) {
        read(s, &buf[0], LEN);
    }

    close(s);
}
BENCHMARK(SocketWrite);

BENCHMARK_MAIN();
