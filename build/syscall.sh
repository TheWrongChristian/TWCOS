#!/bin/sh

. "$1"

output_header

while read num name args
do
	if [ -n "$num" ]
	then
		output_syscall $num $name $args
	fi
done

output_footer
