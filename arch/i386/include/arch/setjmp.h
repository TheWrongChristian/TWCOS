#ifndef ARCH_SETJMP_H
#define ARCH_SETJMP_H

#include <stdint.h>

typedef uint32_t jmp_buf[8];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int value);



#endif /* ARCH_SETJMP_H */
