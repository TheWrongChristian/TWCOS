#!/bin/sh

. "$1"

output_header

while read num return name args
do
	if [ -n "$num" ]
	then
		output_syscall $num $return $name $args
	fi
done

output_footer
