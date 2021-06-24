#ifndef SYS_TYPES_H
#define SYS_TYPES_H

#include <stdint.h>
#include <stddef.h>


typedef int pid_t;
typedef long time_t;
typedef long suseconds_t;
typedef unsigned mode_t;
typedef uintptr_t size_t;
typedef intptr_t ssize_t;
typedef char * ustring;
typedef uint32_t off_t;
typedef uint64_t off64_t;
typedef uint32_t ino_t;
typedef uint64_t ino64_t;

#ifndef __STDC_HOSTED__
#define NEED_TIMESPEC 1
#elif 0==__STDC_HOSTED__
#define NEED_TIMESPEC 1
#endif

#ifdef NEED_TIMESPEC
/* For nanosleep */
struct timespec {
	time_t tv_sec;        /* seconds */
	long   tv_nsec;       /* nanoseconds */
};
#endif
typedef struct timespec * ptimespec;

/* Directory entries */
struct dirent {
	ino_t d_ino;
	off_t d_off;
	unsigned short d_reclen;
	char d_name[1];
};
typedef struct dirent * pdirent;

/* Directory entries */
struct dirent64 {
	ino64_t d_ino;
	off64_t d_off;
	unsigned short d_reclen;
	char d_type;
	char d_name[1];
};
typedef struct dirent64 * pdirent64;

/* For usleep */
typedef uint32_t useconds_t;

#endif
