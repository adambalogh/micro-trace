#include "orig_functions.h"

#define ORIG(func, name)                        \
    do {                                        \
        func = (decltype(func))find_orig(name); \
    } while (0)

void *find_orig(const char *name) { return dlsym(RTLD_NEXT, name); }

OriginalFunctions::OriginalFunctions() {
    ORIG(orig_socket, "socket");
    ORIG(orig_close, "close");
    ORIG(orig_recvfrom, "recvfrom");
    ORIG(orig_accept, "accept");
    ORIG(orig_accept4, "accept4");
    ORIG(orig_recv, "recv");
    ORIG(orig_read, "read");
    ORIG(orig_write, "write");
    ORIG(orig_writev, "writev");
    ORIG(orig_send, "send");
    ORIG(orig_sendto, "sendto");

    ORIG(orig_uv_accept, "uv_accept");
    ORIG(orig_uv_getaddrinfo, "uv_getaddrinfo");
}

OriginalFunctions &orig() {
    static OriginalFunctions o;
    return o;
}
