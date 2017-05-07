#include <assert.h>
#include <stdio.h>

#define FAILED_MSG "Failed: "
     
#define STR(x) #x
#define STRINGIFY(x) STR(x)

#define mu_assert(test) \
    mu_assert_msg(#test, test)

#define mu_assert_msg(msg, test)                              \
    do {                                                      \
        if (!(test)) {                                        \
            printf("Failed at %s: %s\n", __FUNCTION__, msg);  \
            return 1;                                         \
        }                                                     \
    }                                                         \
    while (0)

#define mu_run_test(test) \
    do { \
        int result = test(); tests_run++; \
        if (result == 1) return result; } \
    while (0)

static int all_tests();

int tests_run = 0;

int main(int argc, char** argv) {
    printf("===== Running tests =====\n\n");
    int result = all_tests();
    if (result == 0) {
        printf("%s: ALL TESTS PASSED\n", argv[0]);
    } else {
        printf("%s: TEST FAILED\n", argv[0]);
    }
    printf("Tests run: %d\n", tests_run);
    printf("=========================\n");
    return result != 0;
}

