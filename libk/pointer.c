#include "pointer.h"

#if INTERFACE

#define PTR_BYTE_ADDRESS(base, offset) (void*)(((char*)base)+offset)

#endif
