#include <stdbool.h> /* C doesn't have booleans by default. */
#include <stddef.h>
#include <stdint.h>

#include <setjmp.h>

#include "main.h"

/* Check if the compiler thinks we are targeting the wrong operating system. */
#if 0
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif
 
/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif
#endif
 
void kernel_main() {
	/* Initialize console interface */
	arch_init();

	KTRY {
		thread_init();

		exception_test();
		thread_test();
		tree_test();
		slab_test();

		arch_idle();
	} KCATCH(Throwable) {
		kernel_panic("Error in initialization: %s\n", exception_message());
	}
}
