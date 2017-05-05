#ifndef _HELPERS_H_
#define _HELPERS_H_

// These macros act as a form of documentation
//  - OWNS means that the current struct owns ptr, so it must
//    free it when it's being destroyed.
//  - BORROWS means that the struct just borrowed the ptr, it
//    must not free it.
#define OWNS(ptr) ptr
#define BORROWS(ptr) ptr

// IMPORTANT evaluating expressions a and b multiple times shouldn't
// have any side-effect
#define MIN(a, b) ((a)<(b)) ? (a) : (b)

#endif
