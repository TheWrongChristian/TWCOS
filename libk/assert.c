#include "assert.h"

#if INTERFACE

#define assert(predicate) do { if (!(predicate)) kernel_wait("Assertion failed: " #predicate); } while(0)

#endif
