output_header () {
        cat <<EOF
#include <sys/errno.h>
#include "usyscall.h"

intptr_t syscall_0(int sc)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_1(int sc, intptr_t a1)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc), "b" (a1));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_2(int sc, intptr_t a1, intptr_t a2)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_3(int sc, intptr_t a1, intptr_t a2, intptr_t a3)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_4(int sc, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4)
{
	intptr_t retcode;

	asm("int \$0x80" : "=a" (retcode) : "a" (sc), "b" (a1), "c" (a2), "d" (a3), "S" (a4));

	if (retcode<0) {
		errno = -retcode;
		return -1;
	}

	return retcode;
}

intptr_t syscall_5(int sc, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5)
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
	return syscall_$#($args);
}

EOF
}

output_footer ()
{
	true
}
