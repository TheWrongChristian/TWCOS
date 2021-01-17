#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <sys/types.h>

struct timeval {
       time_t      tv_sec;     /* seconds */
       suseconds_t tv_usec;    /* microseconds */
};

struct timezone {
	int tz_minuteswest;     /* minutes west of Greenwich */
	int tz_dsttime;         /* type of DST correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

#endif
