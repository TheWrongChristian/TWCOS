#include <sys/errno.h>
#include <errno.h>

#include "syscall.h"

#if INTERFACE
#include <stdint.h>

#endif

#if 0
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

reg_t syscall_pii(syscall_e sc, void * p1, int i1, int i2)
{
	return syscall_3(sc, (reg_t)p1, (reg_t)i1, (reg_t)i2);
}

reg_t syscall_pp(syscall_e sc, void * p1, void * p2)
{
	return syscall_2(sc, (reg_t)p1, (reg_t)p2);
}

reg_t syscall_p(syscall_e sc, void * p)
{
	return syscall_1(sc, (reg_t)p);
}

reg_t syscall_i(syscall_e sc, int i1)
{
	return syscall_1(sc, (reg_t)i1);
}

/*
 * The actual system calls
 */
pid_t fork()
{
	return syscall_v(sc_fork);
}

void _exit(int code)
{
	syscall_i(sc_exit, code);
}

pid_t waitpid(pid_t pid, int * wstatus, int options)
{
	return syscall_ipi(sc_waitpid, pid, wstatus, options);
}

pid_t getpid()
{
	return syscall_v(sc_getpid);
}

int execve(char * filename, char * argc[], char * envp[])
{
	return syscall_ppp(sc_execve, filename, argc, envp);
}

ssize_t read(int fd, void *buf, size_t count)
{
	return syscall_ipi(sc_read, fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return syscall_ipi(sc_write, fd, (void*)buf, count);
}

static void * internal_brk(void * p)
{
	return (void*)syscall_p(sc_brk, p);
}
#endif

int brk(void * p)
{
	void * newbrk = internal_brk(p);

	if (newbrk<p) {
		errno = ENOMEM;
		return -1;
	}

	return 0;
}

void * sbrk(intptr_t incr)
{
	char * current = internal_brk(0);
	if (0 == incr) {
		return current;
	}

	return internal_brk(current + incr);
}

#if 0
int open(const char *pathname, int flags, mode_t mode)
{
	return syscall_pii(sc_open, pathname, flags, mode);
}

int close(int fd)
{
	return syscall_i(sc_close, fd);
}

int unlink(const char * path)
{
	return syscall_p(sc_unlink, path);
}
#endif
