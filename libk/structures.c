/*
 * Helper macros for mapping between structures of different types
 */

#if INTERFACE

#include <stddef.h>

#define container_of(p, type, member) ((type*)((char *)(p) - offsetof(type,member)))

#endif
