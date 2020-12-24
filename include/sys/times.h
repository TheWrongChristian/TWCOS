#ifndef SYS_TIMES_H
#define SYS_TIMES_H

#include <stdint.h>

typedef intptr_t clock_t;
struct tms {
        clock_t tms_utime;
        clock_t tms_stime;
        clock_t tms_cutime;
        clock_t tms_cstime;
};

#endif
