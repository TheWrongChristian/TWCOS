#include "syscall.h"

#if INTERFACE

#endif

int errno;

reg_t syscall_0(syscall_e sc)
{
	reg_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

reg_t syscall_1(syscall_e sc, reg_t a1)
{
	reg_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

reg_t syscall_2(syscall_e sc, reg_t a1, reg_t a2)
{
	reg_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

reg_t syscall_3(syscall_e sc, reg_t a1, reg_t a2, reg_t a3)
{
	reg_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

reg_t syscall_4(syscall_e sc, reg_t a1, reg_t a2, reg_t a3, reg_t a4)
{
	reg_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3), "S" (a4));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

reg_t syscall_5(syscall_e sc, reg_t a1, reg_t a2, reg_t a3, reg_t a4, reg_t a5)
{
	reg_t retcode;

	asm("int $0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3), "S" (a4), "D" (a5));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

reg_t syscall_v(syscall_e sc)
{
	return syscall_0(sc);
}

reg_t syscall_ipi(syscall_e sc, int i1, void * p1, int i2)
{
	return syscall_3(sc, (reg_t)i1, (reg_t)p1, (reg_t)i2);
}

reg_t syscall_i(syscall_e sc, int i1)
{
	return syscall_1(sc, (reg_t)i1);
}


ssize_t read(int fd, void *buf, size_t count)
{
	return syscall_ipi(sc_read, fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return syscall_ipi(sc_write, fd, (void*)buf, count);
}
