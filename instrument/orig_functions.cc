#include "orig_functions.h"

extern OriginalFunctions& orig() {
    static OriginalFunctions o;
    return o;
}
