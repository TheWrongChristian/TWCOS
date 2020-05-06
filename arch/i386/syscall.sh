output_header () {
	cat <<EOF
#include "syscall.h"

#if INTERFACE

#include <stdint.h>

typedef intptr_t reg_t;

enum syscall_e {
	/* 0x0 */
	sc_restart, sc_exit, sc_fork, sc_read, sc_write, sc_open, sc_close, sc_waitpid,

	/* 0x8 */
	sc_creat, sc_link, sc_unlink, sc_execve, sc_chdir, sc_time, sc_mknod, sc_chmod,

	/* 0x10 */
	sc_stat = 0x12, sc_lseek, sc_getpid, sc_mount,

	/* 0x18 */
	sc_stime = 0x19, sc_ptrace, sc_alarm, sc_fstat, sc_pause, sc_utime,

	/* 0x20 */
	sc_access = 0x21, sc_nice, sc_sync = 0x24, sc_kill, sc_rename, sc_mkdir,

	/* 0x28 */
	sc_rmdir, sc_dup, sc_pipe, sc_times, sc_brk = 0x2d,

	/* 0x30 */
	sc_signal = 0x30, sc_umount = 0x34, sc_ioctl = 0x36, sc_fcntl, 

	/* 0x38 */
	sc_umask = 0x3c, sc_chroot, sc_ustat, sc_dup2,
};

#define EPERM            1      /* Operation not permitted */
#define ENOENT           2      /* No such file or directory */
#define ESRCH            3      /* No such process */
#define EINTR            4      /* Interrupted system call */
#define EIO              5      /* I/O error */
#define ENXIO            6      /* No such device or address */
#define E2BIG            7      /* Argument list too long */
#define ENOEXEC          8      /* Exec format error */
#define EBADF            9      /* Bad file number */
#define ECHILD          10      /* No child processes */
#define EAGAIN          11      /* Try again */
#define ENOMEM          12      /* Out of memory */
#define EACCES          13      /* Permission denied */
#define EFAULT          14      /* Bad address */
#define ENOTBLK         15      /* Block device required */
#define EBUSY           16      /* Device or resource busy */
#define EEXIST          17      /* File exists */
#define EXDEV           18      /* Cross-device link */
#define ENODEV          19      /* No such device */
#define ENOTDIR         20      /* Not a directory */
#define EISDIR          21      /* Is a directory */
#define EINVAL          22      /* Invalid argument */
#define ENFILE          23      /* File table overflow */
#define EMFILE          24      /* Too many open files */
#define ENOTTY          25      /* Not a typewriter */
#define ETXTBSY         26      /* Text file busy */
#define EFBIG           27      /* File too large */
#define ENOSPC          28      /* No space left on device */
#define ESPIPE          29      /* Illegal seek */
#define EROFS           30      /* Read-only file system */
#define EMLINK          31      /* Too many links */
#define EPIPE           32      /* Broken pipe */
#define EDOM            33      /* Math argument out of domain of func */
#define ERANGE          34      /* Math result not representable */
#define ENOSYS          38      /* Invalid system call number */
#define ENOTEMPTY       39      /* Directory not empty */
#define ELOOP           40      /* Too many symbolic links encountered */
#define EWOULDBLOCK     EAGAIN  /* Operation would block */

#endif

void i386_syscall(uint32_t intr, uint32_t * state)
{
	regs_e argreg=ISR_REG_EAX;
	uint32_t sc = state[argreg];
	reg_t retval;

	KTRY {
		switch(sc) {
EOF
}

output_syscall_args ()
{
	for r in ISR_REG_EBX ISR_REG_ECX ISR_REG_EDX ISR_REG_ESI ISR_REG_EDI
	do
		case "$1/x" in
		/x)
			return
			;;
		*)
			printf "(%s)state[%s] " $1 $r
			;;
		esac
		shift
	done
}

join_args ()
{
	printf "%s" $1
	shift
	for a in "$@"
	do
		printf ",%s" "$a"
	done
}

output_syscall ()
{
	syscall=$1
	name=$2
	shift 2
	case $# in
	0)
		cat <<EOF
		case $syscall:
			retval = sys_$name();
			break;
EOF
		;;
	*)
		args=$(join_args $(output_syscall_args "$@"))
		cat <<EOF
		case $syscall:
			retval = sys_$name($args);
			break;
EOF
		;;
	esac
}

output_footer () {
	cat <<EOF
		default:
			retval = -ENOSYS;
		}
	} KCATCH(Exception) {
		retval = -EINVAL;
	}

	state[ISR_REG_EAX] = retval;
}
EOF
}
