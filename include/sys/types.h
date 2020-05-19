#ifndef SYS_TYPES_H
#define SYS_TYPES_H

#include <stdint.h>


typedef int pid_t;
typedef intptr_t ssize_t;
typedef int time_t;
typedef unsigned mode_t;
typedef const char * const ustring;

/* For nanosleep */
struct timespec {
	time_t tv_sec;        /* seconds */
	long   tv_nsec;       /* nanoseconds */
};
typedef struct timespec * ptimespec;

/* For usleep */
typedef uint32_t useconds_t;

#endif
