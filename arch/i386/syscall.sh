output_header () {
	cat <<EOF
#include <sys/errno.h>
#include <unistd.h>
#include "syscall.h"

#if INTERFACE

#include <stdint.h>

typedef intptr_t reg_t;

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
	returntype=$2
	name=$3
	shift 3
	args=$(join_args $(output_syscall_args "$@"))
	cat <<EOF
		case $syscall:
			retval = sys_$name($args);
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

	state[ISR_REG_EAX] = retval;
}
EOF
}
