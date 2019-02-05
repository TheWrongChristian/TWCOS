#ifndef ARCH_SETJMP_H
#define ARCH_SETJMP_H

#include <stdint.h>

typedef uint32_t jmp_buf[6];

int setjmp(jmp_buf env);
void klongjmp(jmp_buf env, int value);

#define longjmp klongjmp



#endif /* ARCH_SETJMP_H */
