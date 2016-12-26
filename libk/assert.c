#include "assert.h"

#if INTERFACE

#define assert(predicate) do { if (!(predicate)) kernel_panic("Assertion failed: %s\n", #predicate); } while(0)

#endif
