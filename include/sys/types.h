#ifndef SYS_TYPES_H
#define SYS_TYPES_H

#include <stdint.h>
#include <stddef.h>


typedef int pid_t;
typedef int time_t;
typedef unsigned mode_t;
typedef uintptr_t size_t;
typedef intptr_t ssize_t;
typedef const char * const ustring;
typedef int32_t off_t;
typedef int64_t off64_t;
typedef uint32_t ino_t;
typedef uint64_t ino64_t;

/* For nanosleep */
struct timespec {
	time_t tv_sec;        /* seconds */
	long   tv_nsec;       /* nanoseconds */
};
typedef struct timespec * ptimespec;

/* Directory entries */
struct dirent {
	ino_t d_ino;
	off_t d_off;
	size_t d_reclen;
	char d_name[0];
};
typedef struct dirent * pdirent;

/* Directory entries */
struct dirent64 {
	ino_t d_ino;
	off_t d_off;
	size_t d_reclen;
	char d_name[0];
};
typedef struct dirent64 * pdirent64;

/* For usleep */
typedef uint32_t useconds_t;

#endif
