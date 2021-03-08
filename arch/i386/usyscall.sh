output_header () {
        cat <<EOF
#define NEED_TIMESPEC 1

#include <stddef.h>
#include <sys/types.h>
#include <errno.h>
#include "usyscall.h"

#if INTERFACE
#endif

static intptr_t syscall_0(int sc)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

static intptr_t syscall_1(int sc, intptr_t a1)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc), "b" (a1));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

static intptr_t syscall_2(int sc, intptr_t a1, intptr_t a2)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

static intptr_t syscall_3(int sc, intptr_t a1, intptr_t a2, intptr_t a3)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

static intptr_t syscall_4(int sc, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3), "S" (a4));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

static intptr_t syscall_5(int sc, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3), "S" (a4), "D" (a5));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}
EOF
}

output_syscall_args_list ()
{
	for a in $(seq 1 $#)
	do
		printf "%s a%d\n" $1 $a
		shift
	done
}

output_syscall_args ()
{
	echo $1
	shift
	for a in $(seq 1 $#)
	do
		printf "(intptr_t)a%d\n" $a
		shift
	done
}

join_args ()
{
	sep=""
	while read line
	do
		printf "%s%s" "$sep" "$line"
		sep=,
	done
}

output_syscall ()
{
        syscall=$1
	returntype=$2
	name=$3
        shift 3
	args_list=$(output_syscall_args_list "$@" | join_args)
        args=$(output_syscall_args $syscall "$@" | join_args)
        cat <<EOF
$returntype $name($args_list)
{
	return ($returntype)syscall_$#($args);
}

EOF
}

output_footer ()
{
        cat <<EOF
/* Some wrappers */
void _exit(int rc)
{
	(void)doexit(rc);
}

unsigned sleep(time_t seconds)
{
	struct timespec inspec = {seconds, 0};
	struct timespec outspec = {0};
	int r = nanosleep(&inspec, &outspec);
	if (r<0) {
		return outspec.tv_sec;
	}

	return 0;
}

int usleep(useconds_t usec)
{
	struct timespec inspec = {usec/1000000, (usec*1000)%1000000000};

	return nanosleep(&inspec, 0);
}

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

static intptr_t _syscall_fuzz_arg(int seed)
{
	static intptr_t savedseed = 0;
	if (seed) {
		savedseed=seed;
	}
	savedseed = savedseed*1664525 + 1013904223;

	return savedseed;
}

int _syscall_fuzz(int seed)
{
	return syscall_5(_syscall_fuzz_arg(seed) % 256, _syscall_fuzz_arg(0),
		_syscall_fuzz_arg(0), _syscall_fuzz_arg(0),
		_syscall_fuzz_arg(0), _syscall_fuzz_arg(0));
}

EOF
}
