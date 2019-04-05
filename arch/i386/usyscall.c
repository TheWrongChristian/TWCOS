#include "usyscall.h"

intptr_t syscall_0(syscall_e sc)
{
	intptr_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_1(syscall_e sc, intptr_t a1)
{
	intptr_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_2(syscall_e sc, intptr_t a1, intptr_t a2)
{
	intptr_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_3(syscall_e sc, intptr_t a1, intptr_t a2, intptr_t a3)
{
	intptr_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_4(syscall_e sc, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4)
{
	intptr_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3), "S" (a4));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_5(syscall_e sc, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5)
{
	intptr_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3), "S" (a4), "D" (a5));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}
