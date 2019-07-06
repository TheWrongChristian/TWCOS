#include "syscall.h"

#if INTERFACE

#endif

int errno;

reg_t syscall_v(syscall_e sc)
{
	return syscall_0(sc);
}

reg_t syscall_ipi(syscall_e sc, int i1, void * p1, int i2)
{
	return syscall_3(sc, (reg_t)i1, (reg_t)p1, (reg_t)i2);
}

reg_t syscall_ppp(syscall_e sc, void * p1, void * p2, void * p3)
{
	return syscall_3(sc, (reg_t)p1, (reg_t)p2, (reg_t)p3);
}

reg_t syscall_pp(syscall_e sc, void * p1, void * p2)
{
	return syscall_2(sc, (reg_t)p1, (reg_t)p2);
}

reg_t syscall_i(syscall_e sc, int i1)
{
	return syscall_1(sc, (reg_t)i1);
}

reg_t syscall_p(syscall_e sc, void * p1)
{
	return syscall_1(sc, (reg_t)p1);
}


/*
 * The actual system calls
 */
pid_t fork()
{
	return syscall_v(sc_fork);
}

void exit(int code)
{
	syscall_i(sc_exit, code);
}

pid_t waitpid(pid_t pid, int * wstatus, int options)
{
	return syscall_ipi(sc_waitpid, pid, wstatus, options);
}

ssize_t read(int fd, void *buf, size_t count)
{
	return syscall_ipi(sc_read, fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return syscall_ipi(sc_write, fd, (void*)buf, count);
}
