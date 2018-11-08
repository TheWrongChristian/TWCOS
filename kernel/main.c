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

static void idle() {
	while(1) {
		thread_gc();
		thread_yield();
		arch_idle();
	}
}

static void run_init() {
	kernel_printk("In process %d\n", arch_get_thread()->process->pid);
	while(1) {
		thread_yield();
	}
}
 
void kernel_main() {
	/* Initialize console interface */
	arch_init();
	console_writestring("\033[1;1HSimple kernel");
	console_writestring("\033[2;2HSimple kernel");
	console_writestring("\033[3;3HSimple kernel");
	console_writestring("\033[4;4HSimple kernel");
	console_writestring("\033[4S");
	console_writestring("\033[4T");

	KTRY {
		/* Initialize subsystems */
		thread_init();
		slab_init();
		page_cache_init();
		process_init();
		timer_init(arch_timer_ops());
		testshell_init();

		/* Create process 1 - init */
		if (0 == process_fork()) {
			run_init();
		}

		cbuffer_test();
		dtor_test();
		exception_test();
		thread_test();
		tree_test();
		arraymap_test();
		slab_test();
		vector_test();
		arena_test();
		vnode_t * root = tarfs_test();
		vfs_test(root);
		timer_test();

		char * p = arch_heap_page();
		char c = *p;
		*p = 0;
		
		vm_vmpage_trapwrites(vmap_get_page(0, p));
		*p = 0;

		char ** strs = ssplit("/a/path/file/name", '/');
		strs = ssplit("", '/');
		strs = ssplit("/", '/');

		vnode_t * console = dev_vnode(console_dev());
		vnode_t * terminal = terminal_new(console, console);

		idle();
	} KCATCH(Throwable) {
		kernel_panic("Error in initialization: %s\n", exception_message());
	}
}
