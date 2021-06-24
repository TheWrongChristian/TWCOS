output_header () {
	cat <<EOF
#include <sys/errno.h>
#include <unistd.h>
#include "syscall.h"

#if INTERFACE

#include <stdint.h>

typedef intptr_t reg_t;

#endif

void i386_syscall(uint32_t intr, arch_trap_frame_t * state)
{
	const uint32_t sc = state->eax;
	reg_t retval;

	KTRY {
		switch(sc) {
EOF
}

output_syscall_args ()
{
	for r in ebx ecx edx esi edi
	do
		case "$1/x" in
		/x)
			return
			;;
		*)
			printf "(%s)state->%s " $1 $r
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
	returntype=$2
	name=$3
	shift 3
	args=$(join_args $(output_syscall_args "$@"))
	cat <<EOF
		case $syscall:
			retval = (reg_t)sys_$name($args);
			break;
EOF
}

output_footer () {
	cat <<EOF
		default:
			retval = -ENOSYS;
		}
	} KCATCH(Exception) {
		retval = -EINVAL;
	}

	state->eax = retval;
}
EOF
}
