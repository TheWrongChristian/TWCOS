#include "assert.h"

#if INTERFACE

#define assert(predicate) do { if (!(predicate)) kernel_wait("%s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #predicate); } while(0)

#endif
